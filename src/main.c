/**
 * LRA-Calculator-C 主程序文件 / Main Program File
 *
 * 这是一个高性能的音频响度范围 (Loudness Range, LRA) 计算工具的主程序。
 * 程序使用 Grand Central Dispatch (GCD) 实现并发处理，支持批量分析音频文件。
 *
 * This is the main program for a high-performance audio Loudness Range (LRA) calculation tool.
 * The program uses Grand Central Dispatch (GCD) for concurrent processing and supports batch audio file analysis.
 *
 * 主要功能 / Main Features:
 * - 递归扫描目录中的音频文件 / Recursively scan audio files in directories
 * - 并发计算多个文件的 LRA 值 / Concurrently calculate LRA values for multiple files
 * - 支持多种音频格式 / Support multiple audio formats
 * - 生成排序后的结果文件 / Generate sorted result files
 *
 * 作者 / Author: LRA-Calculator-C Development Team
 * 版本 / Version: 2.0
 * 日期 / Date: 2025-08-07
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    // 用于 realpath, sysconf / For realpath, sysconf
#include <sys/stat.h>  // 用于 stat, S_ISDIR / For stat, S_ISDIR
#include <stdatomic.h> // 用于原子操作 / For atomic operations
#include <pthread.h>   // 用于互斥锁 / For mutex
#include <dispatch/dispatch.h> // 用于 Grand Central Dispatch (GCD) / For Grand Central Dispatch (GCD)
#include <errno.h>     // 用于错误处理 / For error handling

#include "utils.h"            // 工具函数 / Utility functions
#include "file_scanner.h"     // 文件扫描模块 (使用 nftw) / File scanner module (uses nftw)
#include "ffmpeg_processor.h" // FFmpeg 处理模块 / FFmpeg processor module
#include "result_manager.h"   // 结果管理模块 / Result manager module

// === 全局共享数据 / Global Shared Data ===
DynamicArrayFiles g_files_to_process;    // 待处理文件列表 / List of files to process
DynamicArrayResults g_processing_results; // 处理结果列表 / List of processing results
pthread_mutex_t g_results_mutex;         // 保护结果列表的互斥锁 / Mutex for protecting results list
atomic_size_t g_processed_file_count;    // 已处理文件计数器 (原子操作) / Processed file counter (atomic)

/**
 * GCD 任务上下文结构体 / GCD Task Context Structure
 *
 * 用于在 GCD 任务之间传递上下文信息，包含文件索引和全局数据指针。
 * Used to pass context information between GCD tasks, including file index and global data pointers.
 */
typedef struct {
    size_t file_index;                    // 文件在列表中的索引 / Index of file in the list
    // 全局数据指针 (GCD 块可以捕获上下文，但显式传递对 C 函数更清晰)
    // Global data pointers (GCD blocks can capture context, but explicit passing is cleaner for C functions)
    DynamicArrayFiles* files_list_ptr;    // 文件列表指针 / Pointer to files list
    DynamicArrayResults* results_list_ptr; // 结果列表指针 / Pointer to results list
    pthread_mutex_t* results_mutex_ptr;   // 互斥锁指针 / Pointer to mutex
    atomic_size_t* processed_count_ptr;   // 处理计数器指针 / Pointer to processed counter
    size_t total_files_for_progress;      // 总文件数 (用于进度显示) / Total files count (for progress display)
} GCDTaskContext;

/**
 * GCD 工作任务函数 / GCD Worker Task Function
 *
 * 这是在 GCD 并发队列中执行的工作函数，负责处理单个音频文件的 LRA 计算。
 * 每个文件都会在独立的任务中处理，实现真正的并行计算。
 *
 * This is the worker function executed in GCD concurrent queue, responsible for
 * processing LRA calculation for a single audio file. Each file is processed
 * in an independent task, achieving true parallel computation.
 *
 * @param context GCD 任务上下文指针 / Pointer to GCD task context
 */
static void process_file_gcd_task_f(void *context) {
    GCDTaskContext *ctx = (GCDTaskContext *)context;

    // 参数验证 / Parameter validation
    if (!ctx) {
        fprintf(stderr, "错误: GCD 任务上下文为空 / Error: GCD task context is null\n");
        return;
    }

    // 获取当前要处理的文件信息 / Get current file information to process
    FileToProcess current_file_info = ctx->files_list_ptr->items[ctx->file_index];
    ProcessResult current_result;

    // 注意: 在 GCD 中，你不能直接管理线程，所以我们省略线程 ID 记录
    // Note: With GCD, you don't manage threads directly, so we omit thread ID logging
    // uintptr_t tid = (uintptr_t)pthread_self(); // 可用于伪 ID / Can be used for pseudo-ID

    // 显示开始处理的进度信息 / Display progress information for start of processing
    size_t current_processed_atomic_val = atomic_load(ctx->processed_count_ptr);
    printf("  [GCD 工作线程 / GCD Worker] (%zu/%zu) 正在分析 / Analyzing: %s\n",
           current_processed_atomic_val + 1, // 即将处理这个文件 / About to process this file
           ctx->total_files_for_progress,
           current_file_info.relative_path);

    // 调用核心 LRA 计算函数 / Call core LRA calculation function
    calculate_lra_for_file(&current_file_info, &current_result);

    // 原子递增已处理文件计数 / Atomically increment processed file count
    atomic_fetch_add(ctx->processed_count_ptr, 1);
    current_processed_atomic_val = atomic_load(ctx->processed_count_ptr);

    // 根据处理结果显示不同的消息 / Display different messages based on processing result
    if (current_result.success) {
        printf("    [GCD 工作线程 / GCD Worker] (%zu/%zu) 成功 / Success: %s LRA: %.1f LU\n",
               current_processed_atomic_val,
               ctx->total_files_for_progress,
               current_result.relative_path, current_result.lra);
    } else {
        printf("    [GCD 工作线程 / GCD Worker] (%zu/%zu) 失败 / Failed: %s 错误 / Error: %s\n",
               current_processed_atomic_val,
               ctx->total_files_for_progress,
               current_result.relative_path, current_result.error_message);
    }

    // 将结果添加到共享列表，使用互斥锁保护 / Add result to shared list, protected by mutex
    pthread_mutex_lock(ctx->results_mutex_ptr);
    if (!DynamicArrayResults_add(ctx->results_list_ptr, &current_result)) {
        fprintf(stderr, "错误: 无法将文件 %s 的结果添加到 GCD 工作线程的结果列表中\n",
                current_file_info.relative_path);
        fprintf(stderr, "Error: Failed to add result for %s to results list from GCD worker\n",
                current_file_info.relative_path);
    }
    pthread_mutex_unlock(ctx->results_mutex_ptr);

    // 释放为此任务分配的上下文内存 / Free the context memory allocated for this task
    free(ctx);
}


/**
 * 程序配置结构体 / Program Configuration Structure
 * 存储用户指定的各种配置选项
 * Stores various configuration options specified by user
 */
typedef struct {
    int interactive_mode;                    // 交互模式标志 / Interactive mode flag
    int verbose_mode;                        // 详细输出模式 / Verbose output mode
    int quiet_mode;                          // 安静模式 / Quiet mode
    char custom_output_file[MAX_PATH_LEN];   // 自定义输出文件名 / Custom output filename
    int max_concurrent_tasks;                // 最大并发任务数 / Maximum concurrent tasks
} ProgramConfig;

/**
 * 显示程序使用帮助信息 / Display program usage help
 */
static void show_usage(const char *program_name) {
    printf("LRA-Calculator-C - 音频响度范围计算器 / Audio Loudness Range Calculator\n");
    printf("版本 / Version: 2.0\n\n");
    printf("用法 / Usage:\n");
    printf("  %s [选项] [目录路径]\n", program_name);
    printf("  %s [options] [directory_path]\n\n", program_name);
    printf("选项 / Options:\n");
    printf("  -h, --help              显示此帮助信息 / Show this help message\n");
    printf("  -v, --version           显示版本信息 / Show version information\n");
    printf("  -i, --interactive       交互模式 (默认) / Interactive mode (default)\n");
    printf("  -q, --quiet             安静模式，减少输出信息 / Quiet mode, reduce output\n");
    printf("  --verbose               详细模式，显示更多调试信息 / Verbose mode, show more debug info\n");
    printf("  -o, --output <文件名>    指定输出文件名 / Specify output filename\n");
    printf("  -j, --jobs <数量>        最大并发任务数 / Maximum concurrent tasks\n\n");
    printf("示例 / Examples:\n");
    printf("  %s                              # 交互模式 / Interactive mode\n", program_name);
    printf("  %s /path/to/music               # 直接指定目录 / Specify directory directly\n", program_name);
    printf("  %s -q /path/to/music            # 安静模式处理 / Process in quiet mode\n", program_name);
    printf("  %s --verbose /path/to/music     # 详细模式处理 / Process in verbose mode\n", program_name);
    printf("  %s -o results.txt /path/to/music # 自定义输出文件 / Custom output file\n", program_name);
    printf("  %s -j 4 /path/to/music          # 限制并发数为4 / Limit concurrency to 4\n", program_name);
    printf("  %s --help                       # 显示帮助 / Show help\n\n", program_name);
    printf("支持的音频格式 / Supported audio formats:\n");
    printf("  WAV, MP3, FLAC, AAC, M4A, OGG, Opus, WMA, AIFF, ALAC\n\n");
    printf("注意事项 / Notes:\n");
    printf("  - 程序需要系统安装 FFmpeg 才能正常工作 / FFmpeg must be installed for the program to work\n");
    printf("  - 大量文件处理可能需要较长时间 / Processing large numbers of files may take considerable time\n");
    printf("  - 并发任务数建议不超过 CPU 核心数的 2 倍 / Concurrent tasks should not exceed 2x CPU cores\n");
}

/**
 * 获取并验证目录路径 / Get and validate directory path
 * @param input_path 输入路径 / Input path
 * @param canonical_path 输出的规范化路径 / Output canonical path
 * @return 成功返回1，失败返回0 / Return 1 on success, 0 on failure
 */
static int get_and_validate_directory(const char *input_path, char *canonical_path) {
    if (!input_path || strlen(input_path) == 0) {
        fprintf(stderr, "错误: 路径不能为空 / Error: Path cannot be empty\n");
        return 0;
    }

    if (realpath(input_path, canonical_path) == NULL) {
        fprintf(stderr, "错误: 无法规范化路径 '%s': %s\n", input_path, strerror(errno));
        fprintf(stderr, "Error: Cannot canonicalize path '%s': %s\n", input_path, strerror(errno));
        return 0;
    }

    struct stat statbuf;
    if (stat(canonical_path, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "错误: \"%s\" 不是一个有效的文件夹路径或文件夹不存在\n", canonical_path);
        fprintf(stderr, "Error: \"%s\" is not a valid directory path or directory does not exist\n", canonical_path);
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    char time_buf[128];
    get_current_time_str(time_buf, sizeof(time_buf));

    char base_folder_input[MAX_PATH_LEN] = {0};
    char base_folder_canonical[MAX_PATH_LEN];
    char results_file_path_abs[MAX_PATH_LEN];

    // 初始化程序配置 / Initialize program configuration
    ProgramConfig config = {
        .interactive_mode = 1,
        .verbose_mode = 0,
        .quiet_mode = 0,
        .custom_output_file = {0},
        .max_concurrent_tasks = 0  // 0 表示使用系统默认值 / 0 means use system default
    };

    // 解析命令行参数 / Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("LRA-Calculator-C 版本 / Version: 2.0\n");
            printf("编译时间 / Build time: %s %s\n", __DATE__, __TIME__);
            printf("支持的功能 / Supported features: GCD并发处理, nftw文件扫描, FFmpeg集成\n");
            printf("Supported features: GCD concurrency, nftw file scanning, FFmpeg integration\n");
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            config.interactive_mode = 1;
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            config.quiet_mode = 1;
            config.verbose_mode = 0;  // 安静模式覆盖详细模式 / Quiet mode overrides verbose mode
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config.verbose_mode = 1;
            config.quiet_mode = 0;    // 详细模式覆盖安静模式 / Verbose mode overrides quiet mode
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                safe_strncpy(config.custom_output_file, argv[++i], sizeof(config.custom_output_file));
            } else {
                fprintf(stderr, "错误: 选项 '%s' 需要一个参数 / Error: Option '%s' requires an argument\n", argv[i], argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "-j") == 0 || strcmp(argv[i], "--jobs") == 0) {
            if (i + 1 < argc) {
                config.max_concurrent_tasks = atoi(argv[++i]);
                if (config.max_concurrent_tasks <= 0) {
                    fprintf(stderr, "错误: 并发任务数必须是正整数 / Error: Number of concurrent tasks must be positive\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "错误: 选项 '%s' 需要一个参数 / Error: Option '%s' requires an argument\n", argv[i], argv[i]);
                return 1;
            }
        } else if (argv[i][0] != '-') {
            // 非选项参数，视为目录路径 / Non-option argument, treat as directory path
            safe_strncpy(base_folder_input, argv[i], sizeof(base_folder_input));
            config.interactive_mode = 0;
        } else {
            fprintf(stderr, "错误: 未知选项 '%s' / Error: Unknown option '%s'\n", argv[i], argv[i]);
            fprintf(stderr, "使用 '%s --help' 查看帮助信息 / Use '%s --help' for help\n", argv[0], argv[0]);
            return 1;
        }
    }

    // 根据配置显示欢迎信息 / Display welcome message based on configuration
    if (!config.quiet_mode) {
        printf("欢迎使用音频 LRA 计算器 (C语言版 V2.0 - GCD & NFTW)！\n");
        printf("Welcome to Audio LRA Calculator (C Version V2.0 - GCD & NFTW)!\n");
        printf("当前时间 / Current time: %s\n", time_buf);

        if (config.verbose_mode) {
            printf("详细模式已启用 / Verbose mode enabled\n");
            if (config.max_concurrent_tasks > 0) {
                printf("最大并发任务数 / Max concurrent tasks: %d\n", config.max_concurrent_tasks);
            } else {
                printf("最大并发任务数 / Max concurrent tasks: 系统自动 / System auto\n");
            }
            if (strlen(config.custom_output_file) > 0) {
                printf("自定义输出文件 / Custom output file: %s\n", config.custom_output_file);
            }
        }
        printf("\n");
    }

    // 获取目录路径 / Get directory path
    if (config.interactive_mode || strlen(base_folder_input) == 0) {
        while (1) {
            printf("请输入要递归处理的音乐顶层文件夹路径 / Enter the top-level music folder path to process recursively: ");
            if (fgets(base_folder_input, sizeof(base_folder_input), stdin) == NULL) {
                fprintf(stderr, "错误: 读取输入失败 / Error: Failed to read input\n");
                return 1;
            }
            base_folder_input[strcspn(base_folder_input, "\n")] = 0;

            if (get_and_validate_directory(base_folder_input, base_folder_canonical)) {
                break;
            }
            printf("请重新输入 / Please try again\n\n");
        }
    } else {
        if (!get_and_validate_directory(base_folder_input, base_folder_canonical)) {
            return 1;
        }
    }

    // 确定结果文件路径 / Determine result file path
    if (strlen(config.custom_output_file) > 0) {
        // 使用自定义输出文件 / Use custom output file
        if (config.custom_output_file[0] == '/') {
            // 绝对路径 / Absolute path
            safe_strncpy(results_file_path_abs, config.custom_output_file, sizeof(results_file_path_abs));
        } else {
            // 相对路径，相对于扫描目录 / Relative path, relative to scan directory
            snprintf(results_file_path_abs, sizeof(results_file_path_abs),
                    "%s/%s", base_folder_canonical, config.custom_output_file);
        }
    } else {
        // 使用默认文件名 / Use default filename
        snprintf(results_file_path_abs, sizeof(results_file_path_abs),
                "%s/%s", base_folder_canonical, LRA_RESULTS_FILENAME);
    }

    if (!config.quiet_mode) {
        printf("正在递归扫描文件夹 / Recursively scanning directory: %s (使用 nftw / using nftw)\n", base_folder_canonical);
        if (config.verbose_mode) {
            printf("结果将保存到 / Results will be saved to: %s\n", results_file_path_abs);
        }
    }

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