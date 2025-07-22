#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    // For realpath, sysconf
#include <sys/stat.h>  // For stat, S_ISDIR
#include <stdatomic.h> // For atomic_size_t
#include <pthread.h>   // For pthread_mutex_t
#include <dispatch/dispatch.h> // For Grand Central Dispatch (GCD)
#include <errno.h>

#include "utils.h"
#include "file_scanner.h"     // Uses nftw
#include "ffmpeg_processor.h" // For calculate_lra_for_file
#include "result_manager.h"

// --- Global shared data ---
DynamicArrayFiles g_files_to_process;
DynamicArrayResults g_processing_results;
pthread_mutex_t g_results_mutex;      // Mutex for g_processing_results
atomic_size_t g_processed_file_count; // Atomic counter for progress

// Context for GCD tasks
typedef struct {
    size_t file_index; // Index into g_files_to_process
    // Pointers to global data (GCD blocks capture context, but explicit passing is cleaner for C functions)
    DynamicArrayFiles* files_list_ptr;
    DynamicArrayResults* results_list_ptr;
    pthread_mutex_t* results_mutex_ptr;
    atomic_size_t* processed_count_ptr;
    size_t total_files_for_progress;
} GCDTaskContext;

// GCD worker function
void process_file_gcd_task_f(void *context) {
    GCDTaskContext *ctx = (GCDTaskContext *)context;

    FileToProcess current_file_info = ctx->files_list_ptr->items[ctx->file_index];
    ProcessResult current_result;

    // It's good practice to have a unique thread identifier for logging,
    // but with GCD, you don't manage threads directly. For simplicity, we'll omit it.
    // uintptr_t tid = (uintptr_t)pthread_self(); // Can be used for a pseudo-ID

    size_t current_processed_atomic_val = atomic_load(ctx->processed_count_ptr);
    printf("  [GCD Worker] (%zu/%zu) Analyzing: %s\n",
           current_processed_atomic_val + 1, // About to process this one
           ctx->total_files_for_progress,
           current_file_info.relative_path);

    calculate_lra_for_file(&current_file_info, &current_result);

    atomic_fetch_add(ctx->processed_count_ptr, 1); // Increment after processing
    current_processed_atomic_val = atomic_load(ctx->processed_count_ptr);


    if (current_result.success) {
        printf("    [GCD Worker] (%zu/%zu) Success: %s LRA: %.1f LU\n",
               current_processed_atomic_val,
               ctx->total_files_for_progress,
               current_result.relative_path, current_result.lra);
    } else {
        printf("    [GCD Worker] (%zu/%zu) Failed: %s Error: %s\n",
               current_processed_atomic_val,
               ctx->total_files_for_progress,
               current_result.relative_path, current_result.error_message);
    }

    // Add result to shared list, protected by mutex
    pthread_mutex_lock(ctx->results_mutex_ptr);
    if (!DynamicArrayResults_add(ctx->results_list_ptr, &current_result)) {
        fprintf(stderr, "Error: Failed to add result for %s to results list from GCD worker.\n", current_file_info.relative_path);
    }
    pthread_mutex_unlock(ctx->results_mutex_ptr);

    free(ctx); // Free the context malloc'ed for this task
}


int main() {
    char time_buf[128];
    get_current_time_str(time_buf, sizeof(time_buf));
    printf("欢迎使用音频 LRA 计算器 (C语言版 V2.0 - GCD & NFTW)！\n");
    printf("当前时间: %s\n", time_buf);

    char base_folder_input[MAX_PATH_LEN];
    char base_folder_canonical[MAX_PATH_LEN];
    char results_file_path_abs[MAX_PATH_LEN];

    while (1) {
        printf("请输入要递归处理的音乐顶层文件夹路径: ");
        if (fgets(base_folder_input, sizeof(base_folder_input), stdin) == NULL) {
            fprintf(stderr, "错误: 读取输入失败。\n"); return 1;
        }
        base_folder_input[strcspn(base_folder_input, "\n")] = 0;
        if (strlen(base_folder_input) == 0) {
            fprintf(stderr, "错误: 路径不能为空，请重新输入。\n"); continue;
        }
        if (realpath(base_folder_input, base_folder_canonical) == NULL) {
            fprintf(stderr, "错误: 无法规范化路径 '%s': %s.\n", base_folder_input, strerror(errno)); continue;
        }
        struct stat statbuf;
        if (stat(base_folder_canonical, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode)) {
            fprintf(stderr, "错误: \"%s\" 不是一个有效的文件夹路径或文件夹不存在。\n", base_folder_canonical); continue;
        }
        break;
    }

    snprintf(results_file_path_abs, sizeof(results_file_path_abs), "%s/%s", base_folder_canonical, LRA_RESULTS_FILENAME);

    printf("正在递归扫描文件夹: %s (使用 nftw)\n", base_folder_canonical);

    if (!DynamicArrayFiles_init(&g_files_to_process, 128)) return 1;
    if (!DynamicArrayResults_init(&g_processing_results, 128)) {
        DynamicArrayFiles_free(&g_files_to_process); return 1;
    }
    if (pthread_mutex_init(&g_results_mutex, NULL) != 0) {
        perror("Failed to initialize results mutex");
        DynamicArrayFiles_free(&g_files_to_process); DynamicArrayResults_free(&g_processing_results); return 1;
    }
    atomic_init(&g_processed_file_count, 0);

    if (!scan_files_with_nftw(base_folder_canonical, &g_files_to_process, base_folder_canonical, results_file_path_abs)) {
        fprintf(stderr, "文件扫描过程中发生错误 (nftw)。\n");
        // Fall through for cleanup
    }

    if (g_files_to_process.count == 0) {
        printf("在指定路径下没有找到支持的音频文件。\n");
        FILE* empty_results_file = fopen(results_file_path_abs, "w");
        if (empty_results_file) {
            fprintf(empty_results_file, "%s\n", LRA_HEADER_LINE);
            fclose(empty_results_file);
        } else {
            fprintf(stderr, "无法创建空的结果文件 %s\n", results_file_path_abs);
        }
    } else {
        printf("扫描完成，找到 %zu 个音频文件待处理。\n", g_files_to_process.count);
        printf("开始并行分析 (使用 Grand Central Dispatch)...\n");

        dispatch_queue_t global_concurrent_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        dispatch_group_t processing_group = dispatch_group_create();

        for (size_t i = 0; i < g_files_to_process.count; ++i) {
            // Create context for each task - this context will be freed by the worker
            GCDTaskContext *task_ctx = (GCDTaskContext*)malloc(sizeof(GCDTaskContext));
            if (!task_ctx) {
                fprintf(stderr, "严重错误: 无法为GCD任务分配内存 (文件索引 %zu)。跳过此文件。\n", i);
                // Potentially mark this file as failed or reduce total count
                continue;
            }
            task_ctx->file_index = i;
            task_ctx->files_list_ptr = &g_files_to_process;
            task_ctx->results_list_ptr = &g_processing_results;
            task_ctx->results_mutex_ptr = &g_results_mutex;
            task_ctx->processed_count_ptr = &g_processed_file_count;
            task_ctx->total_files_for_progress = g_files_to_process.count;

            dispatch_group_async_f(processing_group, global_concurrent_queue, task_ctx, process_file_gcd_task_f);
        }

        // Wait for all tasks in the group to complete
        dispatch_group_wait(processing_group, DISPATCH_TIME_FOREVER);

        // Release GCD objects (GCD uses reference counting)
        // dispatch_release(processing_group); // In modern Objective-C with ARC, this is automatic.
        // For pure C, it depends on the libdispatch version and how it's built/linked.
        // On Apple platforms, dispatch_release is often not needed for objects returned by get_...
        // or create... functions if ARC is not involved.
        // However, `dispatch_group_create` returns a +1 retain count object.
        // It's safer to release it. If using a version of libdispatch where this causes issues
        // (e.g. ARC-like behavior even in C), this might need to be omitted.
        // On macOS, it's typically required for objects from `_create` functions.
#if !OS_OBJECT_USE_OBJC // If not using Objective-C object semantics for GCD objects
        if (processing_group) dispatch_release(processing_group);
#endif
        // Global queues don't need to be released.

        printf("\n并行分析阶段完成。\n");

        if (!write_results_to_file(results_file_path_abs, &g_processing_results, LRA_HEADER_LINE)) {
            fprintf(stderr, "结果写入文件失败。\n");
        } else {
            printf("结果写入完成。\n");
        }

        size_t actual_successes = 0;
        size_t actual_failures = 0;
        for(size_t i = 0; i < g_processing_results.count; ++i) {
            if(g_processing_results.items[i].success) actual_successes++;
            else actual_failures++;
        }
        printf("成功处理 %zu 个文件。\n", actual_successes);
        if (actual_failures > 0) {
            printf("%zu 个文件处理失败。详情如下:\n", actual_failures);
            for(size_t i = 0; i < g_processing_results.count; ++i) {
                if(!g_processing_results.items[i].success) {
                    fprintf(stderr, "  - 文件 '%s': %s\n", g_processing_results.items[i].relative_path, g_processing_results.items[i].error_message);
                }
            }
        }

        if (actual_successes > 0) {
            if (!sort_results_file(results_file_path_abs, LRA_HEADER_LINE)) {
                fprintf(stderr, "错误：排序结果文件 %s 失败。\n", results_file_path_abs);
            }
        } else {
            printf("没有成功处理的文件，跳过排序。\n");
        }
    }

    DynamicArrayFiles_free(&g_files_to_process);
    DynamicArrayResults_free(&g_processing_results);
    pthread_mutex_destroy(&g_results_mutex);

    get_current_time_str(time_buf, sizeof(time_buf));
    printf("所有操作完成！结果已保存于: %s\n", results_file_path_abs);
    printf("结束时间: %s\n", time_buf);

    return 0;
}