#include <dispatch/dispatch.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cache_manager.h"
#include "ffmpeg_processor.h"
#include "file_scanner.h"
#include "result_manager.h"
#include "utils.h"

DynamicArrayFiles g_files_to_process;
DynamicArrayResults g_processing_results;
pthread_mutex_t g_results_mutex;
atomic_size_t g_processed_file_count;

typedef struct {
    size_t file_index;
    DynamicArrayFiles *files_list_ptr;
    DynamicArrayResults *results_list_ptr;
    pthread_mutex_t *results_mutex_ptr;
    atomic_size_t *processed_count_ptr;
    size_t total_files_for_progress;
    int quiet_mode;
    int retry_count;
    FFmpegExecutionConfig ffmpeg_config;
    dispatch_semaphore_t throttle_semaphore;
    int throttle_enabled;
} GCDTaskContext;

typedef struct {
    int interactive_mode;
    int verbose_mode;
    int quiet_mode;
    int resume_mode;
    int retry_count;
    int max_concurrent_tasks;
    int generate_failure_report;
    OutputFormat output_format;
    FFmpegExecutionConfig ffmpeg_config;
    char custom_output_file[MAX_PATH_LEN];
    char custom_cache_file[MAX_PATH_LEN];
    char custom_failure_report_file[MAX_PATH_LEN];
    char include_pattern[MAX_PATH_LEN];
    char exclude_pattern[MAX_PATH_LEN];
} ProgramConfig;

static int path_is_within_base(const char *base_path, const char *target_path) {
    size_t base_len = strlen(base_path);
    if (strncmp(base_path, target_path, base_len) != 0) {
        return 0;
    }

    if (target_path[base_len] == '\0') return 1;
    return target_path[base_len] == '/';
}

static int parse_int_option(const char *text, int minimum_value, int *out_value) {
    if (!text || !out_value) return 0;

    char *end_ptr = NULL;
    errno = 0;
    long value = strtol(text, &end_ptr, 10);
    if (errno != 0 || end_ptr == text || *end_ptr != '\0') return 0;
    if (value < minimum_value || value > 1000000) return 0;

    *out_value = (int)value;
    return 1;
}

static const char *default_results_filename(OutputFormat format) {
    switch (format) {
        case OUTPUT_FORMAT_CSV:
            return "lra_results.csv";
        case OUTPUT_FORMAT_JSON:
            return "lra_results.json";
        case OUTPUT_FORMAT_TEXT:
        default:
            return LRA_RESULTS_FILENAME;
    }
}

static int resolve_output_path_inside_base(const char *base_folder_canonical,
                                           const char *requested_path,
                                           char *resolved_output_path,
                                           size_t resolved_output_size,
                                           char *error_message,
                                           size_t error_message_size) {
    if (!base_folder_canonical || !requested_path || !resolved_output_path) return 0;

    if (strchr(requested_path, '\n') || strchr(requested_path, '\r')) {
        snprintf(error_message, error_message_size,
                 "输出路径包含非法换行字符 / Output path contains illegal newline characters");
        return 0;
    }

    char candidate[MAX_PATH_LEN];
    if (requested_path[0] == '/') {
        safe_strncpy(candidate, requested_path, sizeof(candidate));
    } else {
        int written = snprintf(candidate, sizeof(candidate), "%s/%s",
                               base_folder_canonical, requested_path);
        if (written < 0 || written >= (int)sizeof(candidate)) {
            snprintf(error_message, error_message_size,
                     "输出路径过长 / Output path too long");
            return 0;
        }
    }

    char parent_path[MAX_PATH_LEN];
    char file_name[MAX_PATH_LEN];

    const char *slash = strrchr(candidate, '/');
    if (!slash) {
        safe_strncpy(parent_path, ".", sizeof(parent_path));
        safe_strncpy(file_name, candidate, sizeof(file_name));
    } else {
        size_t parent_len = (size_t)(slash - candidate);
        if (parent_len == 0) {
            safe_strncpy(parent_path, "/", sizeof(parent_path));
        } else {
            if (parent_len >= sizeof(parent_path)) {
                snprintf(error_message, error_message_size,
                         "输出路径父目录过长 / Parent directory is too long");
                return 0;
            }
            memcpy(parent_path, candidate, parent_len);
            parent_path[parent_len] = '\0';
        }
        safe_strncpy(file_name, slash + 1, sizeof(file_name));
    }

    if (file_name[0] == '\0') {
        snprintf(error_message, error_message_size,
                 "输出文件名为空 / Output filename is empty");
        return 0;
    }

    char parent_canonical[MAX_PATH_LEN];
    if (!realpath(parent_path, parent_canonical)) {
        snprintf(error_message, error_message_size,
                 "无法解析输出目录 '%s': %s", parent_path, strerror(errno));
        return 0;
    }

    int final_written = snprintf(resolved_output_path, resolved_output_size,
                                 "%s/%s", parent_canonical, file_name);
    if (final_written < 0 || final_written >= (int)resolved_output_size) {
        snprintf(error_message, error_message_size,
                 "输出路径过长 / Output path too long");
        return 0;
    }

    if (!path_is_within_base(base_folder_canonical, resolved_output_path)) {
        snprintf(error_message, error_message_size,
                 "输出路径必须位于扫描目录内部 / Output path must stay within scan directory");
        return 0;
    }

    return 1;
}

static int get_and_validate_directory(const char *input_path, char *canonical_path) {
    if (!input_path || strlen(input_path) == 0) {
        fprintf(stderr, "错误: 路径不能为空 / Error: Path cannot be empty\n");
        return 0;
    }

    if (realpath(input_path, canonical_path) == NULL) {
        fprintf(stderr, "错误: 无法规范化路径 '%s': %s\n", input_path, strerror(errno));
        return 0;
    }

    struct stat statbuf;
    if (stat(canonical_path, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "错误: \"%s\" 不是一个有效的文件夹路径或文件夹不存在\n",
                canonical_path);
        return 0;
    }

    return 1;
}

static int parse_output_format(const char *format_text, OutputFormat *format) {
    if (!format_text || !format) return 0;
    if (strcmp(format_text, "txt") == 0 || strcmp(format_text, "text") == 0) {
        *format = OUTPUT_FORMAT_TEXT;
        return 1;
    }
    if (strcmp(format_text, "csv") == 0) {
        *format = OUTPUT_FORMAT_CSV;
        return 1;
    }
    if (strcmp(format_text, "json") == 0) {
        *format = OUTPUT_FORMAT_JSON;
        return 1;
    }
    return 0;
}

static size_t detect_default_concurrency(void) {
    long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpu_count <= 0) return 2;
    size_t recommended = (size_t)cpu_count * 2;
    return recommended > 0 ? recommended : 2;
}

static const ProcessResult *find_success_result_by_relative_path(
        const DynamicArrayResults *results, const char *relative_path) {
    if (!results || !relative_path) return NULL;
    for (size_t i = 0; i < results->count; ++i) {
        if (results->items[i].success &&
            strcmp(results->items[i].relative_path, relative_path) == 0) {
            return &results->items[i];
        }
    }
    return NULL;
}

static void show_usage(const char *program_name) {
    printf("LRA-Calculator-C - 音频响度范围计算器 / Audio Loudness Range Calculator\n");
    printf("版本 / Version: 2.1\n\n");
    printf("用法 / Usage:\n");
    printf("  %s [选项] [目录路径]\n", program_name);
    printf("  %s [options] [directory_path]\n\n", program_name);
    printf("选项 / Options:\n");
    printf("  -h, --help                         显示帮助 / Show help\n");
    printf("  -v, --version                      显示版本 / Show version\n");
    printf("  -i, --interactive                  交互模式 / Interactive mode\n");
    printf("  -q, --quiet                        安静模式 / Quiet mode\n");
    printf("  --verbose                          详细输出 / Verbose logs\n");
    printf("  -o, --output <path>                输出文件路径 (必须在扫描目录内)\n");
    printf("  -j, --jobs <N>                     最大并发任务数\n");
    printf("  --format <txt|csv|json>            结果输出格式\n");
    printf("  --include <glob>                   只处理匹配 glob 的相对路径\n");
    printf("  --exclude <glob>                   排除匹配 glob 的相对路径\n");
    printf("  --resume                           启用增量缓存恢复\n");
    printf("  --cache <path>                     缓存文件路径 (必须在扫描目录内)\n");
    printf("  --retry <N>                        失败重试次数 (默认 0)\n");
    printf("  --ffmpeg-timeout <sec>             单文件超时秒数 (默认 120)\n");
    printf("  --ffmpeg-max-cpu <sec>             单文件 CPU 限制秒数 (默认 120)\n");
    printf("  --ffmpeg-max-memory-mb <mb>        单文件进程内存上限 MB (默认 1024)\n");
    printf("  --failure-report <path>            失败报告路径 (必须在扫描目录内)\n");
    printf("  --no-failure-report                禁止写失败报告\n\n");
}

static void process_file_gcd_task_f(void *context) {
    GCDTaskContext *ctx = (GCDTaskContext *)context;
    if (!ctx) return;

    FileToProcess current_file_info = ctx->files_list_ptr->items[ctx->file_index];
    ProcessResult current_result;

    if (!ctx->quiet_mode) {
        size_t current_processed_atomic_val = atomic_load(ctx->processed_count_ptr);
        printf("  [Worker] (%zu/%zu) Analyzing: %s\n",
               current_processed_atomic_val + 1,
               ctx->total_files_for_progress,
               current_file_info.relative_path);
    }

    int total_attempts = ctx->retry_count + 1;
    for (int attempt = 1; attempt <= total_attempts; ++attempt) {
        calculate_lra_for_file(&current_file_info, &current_result, &ctx->ffmpeg_config);
        if (current_result.success) break;

        if (attempt < total_attempts && !ctx->quiet_mode) {
            printf("    [Worker] Retry %d/%d for %s\n",
                   attempt,
                   ctx->retry_count,
                   current_file_info.relative_path);
        }
    }

    atomic_fetch_add(ctx->processed_count_ptr, 1);
    size_t done = atomic_load(ctx->processed_count_ptr);

    if (!ctx->quiet_mode) {
        if (current_result.success) {
            printf("    [Worker] (%zu/%zu) Success: %s LRA: %.1f LU\n",
                   done,
                   ctx->total_files_for_progress,
                   current_result.relative_path,
                   current_result.lra);
        } else {
            printf("    [Worker] (%zu/%zu) Failed: %s Error: %s\n",
                   done,
                   ctx->total_files_for_progress,
                   current_result.relative_path,
                   current_result.error_message);
        }
    }

    pthread_mutex_lock(ctx->results_mutex_ptr);
    if (!DynamicArrayResults_add(ctx->results_list_ptr, &current_result)) {
        fprintf(stderr, "Error: Failed to add result for %s\n", current_file_info.relative_path);
    }
    pthread_mutex_unlock(ctx->results_mutex_ptr);

    if (ctx->throttle_enabled) {
        dispatch_semaphore_signal(ctx->throttle_semaphore);
    }

    free(ctx);
}

int main(int argc, char *argv[]) {
    char time_buf[128];
    get_current_time_str(time_buf, sizeof(time_buf));

    char base_folder_input[MAX_PATH_LEN] = {0};
    char base_folder_canonical[MAX_PATH_LEN];
    char results_file_path_abs[MAX_PATH_LEN] = {0};
    char cache_file_path_abs[MAX_PATH_LEN] = {0};
    char failure_report_path_abs[MAX_PATH_LEN] = {0};

    ProgramConfig config = {
        .interactive_mode = 1,
        .verbose_mode = 0,
        .quiet_mode = 0,
        .resume_mode = 0,
        .retry_count = 0,
        .max_concurrent_tasks = 0,
        .generate_failure_report = 1,
        .output_format = OUTPUT_FORMAT_TEXT,
        .ffmpeg_config = {
            .timeout_seconds = 120,
            .max_cpu_seconds = 120,
            .max_memory_mb = 1024
        },
        .custom_output_file = {0},
        .custom_cache_file = {0},
        .custom_failure_report_file = {0},
        .include_pattern = {0},
        .exclude_pattern = {0}
    };

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("LRA-Calculator-C 版本 / Version: 2.1\n");
            printf("Build: %s %s\n", __DATE__, __TIME__);
            printf("Features: safe exec, retry, cache, glob filters, txt/csv/json output\n");
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            config.interactive_mode = 1;
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            config.quiet_mode = 1;
            config.verbose_mode = 0;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config.verbose_mode = 1;
            config.quiet_mode = 0;
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                safe_strncpy(config.custom_output_file, argv[++i], sizeof(config.custom_output_file));
            } else {
                fprintf(stderr, "Error: Option '%s' requires argument\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "-j") == 0 || strcmp(argv[i], "--jobs") == 0) {
            if (i + 1 >= argc || !parse_int_option(argv[++i], 1, &config.max_concurrent_tasks)) {
                fprintf(stderr, "Error: --jobs requires a positive integer\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--format") == 0) {
            if (i + 1 >= argc || !parse_output_format(argv[++i], &config.output_format)) {
                fprintf(stderr, "Error: --format must be txt/csv/json\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--include") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --include requires a glob pattern\n");
                return 1;
            }
            safe_strncpy(config.include_pattern, argv[++i], sizeof(config.include_pattern));
        } else if (strcmp(argv[i], "--exclude") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --exclude requires a glob pattern\n");
                return 1;
            }
            safe_strncpy(config.exclude_pattern, argv[++i], sizeof(config.exclude_pattern));
        } else if (strcmp(argv[i], "--resume") == 0) {
            config.resume_mode = 1;
        } else if (strcmp(argv[i], "--cache") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --cache requires a path\n");
                return 1;
            }
            safe_strncpy(config.custom_cache_file, argv[++i], sizeof(config.custom_cache_file));
        } else if (strcmp(argv[i], "--retry") == 0) {
            if (i + 1 >= argc || !parse_int_option(argv[++i], 0, &config.retry_count)) {
                fprintf(stderr, "Error: --retry requires a non-negative integer\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--ffmpeg-timeout") == 0) {
            if (i + 1 >= argc || !parse_int_option(argv[++i], 1, &config.ffmpeg_config.timeout_seconds)) {
                fprintf(stderr, "Error: --ffmpeg-timeout requires a positive integer\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--ffmpeg-max-cpu") == 0) {
            if (i + 1 >= argc || !parse_int_option(argv[++i], 1, &config.ffmpeg_config.max_cpu_seconds)) {
                fprintf(stderr, "Error: --ffmpeg-max-cpu requires a positive integer\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--ffmpeg-max-memory-mb") == 0) {
            int mem_mb = 0;
            if (i + 1 >= argc || !parse_int_option(argv[++i], 32, &mem_mb)) {
                fprintf(stderr, "Error: --ffmpeg-max-memory-mb requires integer >= 32\n");
                return 1;
            }
            config.ffmpeg_config.max_memory_mb = (size_t)mem_mb;
        } else if (strcmp(argv[i], "--failure-report") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --failure-report requires a path\n");
                return 1;
            }
            safe_strncpy(config.custom_failure_report_file, argv[++i],
                         sizeof(config.custom_failure_report_file));
        } else if (strcmp(argv[i], "--no-failure-report") == 0) {
            config.generate_failure_report = 0;
        } else if (argv[i][0] != '-') {
            safe_strncpy(base_folder_input, argv[i], sizeof(base_folder_input));
            config.interactive_mode = 0;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            return 1;
        }
    }

    if (!config.quiet_mode) {
        printf("Welcome to Audio LRA Calculator (C Version V2.1)!\n");
        printf("Current time: %s\n", time_buf);
    }

    if (config.interactive_mode || strlen(base_folder_input) == 0) {
        while (1) {
            printf("Enter directory path: ");
            if (!fgets(base_folder_input, sizeof(base_folder_input), stdin)) {
                fprintf(stderr, "Error: Failed to read input\n");
                return 1;
            }
            base_folder_input[strcspn(base_folder_input, "\n")] = '\0';
            if (get_and_validate_directory(base_folder_input, base_folder_canonical)) {
                break;
            }
            printf("Please try again.\n");
        }
    } else {
        if (!get_and_validate_directory(base_folder_input, base_folder_canonical)) {
            return 1;
        }
    }

    {
        char error_buf[MAX_LINE_LEN] = {0};
        const char *requested_output = config.custom_output_file[0] != '\0'
                                       ? config.custom_output_file
                                       : default_results_filename(config.output_format);

        if (!resolve_output_path_inside_base(base_folder_canonical, requested_output,
                                             results_file_path_abs,
                                             sizeof(results_file_path_abs),
                                             error_buf,
                                             sizeof(error_buf))) {
            fprintf(stderr, "Error: %s\n", error_buf);
            return 1;
        }

        if (config.resume_mode) {
            const char *requested_cache = config.custom_cache_file[0] != '\0'
                                          ? config.custom_cache_file
                                          : ".lra_cache.tsv";
            if (!resolve_output_path_inside_base(base_folder_canonical, requested_cache,
                                                 cache_file_path_abs,
                                                 sizeof(cache_file_path_abs),
                                                 error_buf,
                                                 sizeof(error_buf))) {
                fprintf(stderr, "Error: %s\n", error_buf);
                return 1;
            }
        }

        if (config.generate_failure_report) {
            const char *requested_failure = config.custom_failure_report_file[0] != '\0'
                                            ? config.custom_failure_report_file
                                            : "lra_failures.txt";
            if (!resolve_output_path_inside_base(base_folder_canonical, requested_failure,
                                                 failure_report_path_abs,
                                                 sizeof(failure_report_path_abs),
                                                 error_buf,
                                                 sizeof(error_buf))) {
                fprintf(stderr, "Error: %s\n", error_buf);
                return 1;
            }
        }
    }

    if (!DynamicArrayFiles_init(&g_files_to_process, 128)) return 1;
    if (!DynamicArrayResults_init(&g_processing_results, 128)) {
        DynamicArrayFiles_free(&g_files_to_process);
        return 1;
    }
    if (pthread_mutex_init(&g_results_mutex, NULL) != 0) {
        perror("Failed to initialize results mutex");
        DynamicArrayFiles_free(&g_files_to_process);
        DynamicArrayResults_free(&g_processing_results);
        return 1;
    }
    atomic_init(&g_processed_file_count, 0);

    ScanOptions scan_options = {
        .include_pattern = config.include_pattern[0] != '\0' ? config.include_pattern : NULL,
        .exclude_pattern = config.exclude_pattern[0] != '\0' ? config.exclude_pattern : NULL
    };

    if (!scan_files_with_nftw(base_folder_canonical, &g_files_to_process, base_folder_canonical,
                              results_file_path_abs, &scan_options)) {
        fprintf(stderr, "Error: directory scan failed\n");
    }

    DynamicArrayFiles all_scanned_files;
    if (!DynamicArrayFiles_init(&all_scanned_files, g_files_to_process.count + 8)) {
        fprintf(stderr, "Error: cannot allocate all_scanned_files\n");
        DynamicArrayFiles_free(&g_files_to_process);
        DynamicArrayResults_free(&g_processing_results);
        pthread_mutex_destroy(&g_results_mutex);
        return 1;
    }

    for (size_t i = 0; i < g_files_to_process.count; ++i) {
        DynamicArrayFiles_add(&all_scanned_files, &g_files_to_process.items[i]);
    }

    if (config.resume_mode) {
        DynamicArrayCacheEntries cache_entries;
        DynamicArrayFiles pending_files;

        if (!DynamicArrayCacheEntries_init(&cache_entries, g_files_to_process.count + 8) ||
            !DynamicArrayFiles_init(&pending_files, g_files_to_process.count + 8)) {
            fprintf(stderr, "Error: cannot allocate cache helpers\n");
            DynamicArrayFiles_free(&all_scanned_files);
            DynamicArrayFiles_free(&g_files_to_process);
            DynamicArrayResults_free(&g_processing_results);
            pthread_mutex_destroy(&g_results_mutex);
            return 1;
        }

        if (!load_cache_file(cache_file_path_abs, &cache_entries)) {
            fprintf(stderr, "Warning: cache load failed, continuing without cache\n");
        }

        size_t cached_hits = 0;
        for (size_t i = 0; i < g_files_to_process.count; ++i) {
            const FileToProcess *file = &g_files_to_process.items[i];
            const CacheEntry *entry = find_cache_entry(&cache_entries, file);
            if (entry && isfinite(entry->lra)) {
                ProcessResult cached_result;
                safe_strncpy(cached_result.relative_path, file->relative_path,
                             sizeof(cached_result.relative_path));
                cached_result.lra = entry->lra;
                cached_result.success = 1;
                cached_result.error_message[0] = '\0';
                DynamicArrayResults_add(&g_processing_results, &cached_result);
                cached_hits++;
            } else {
                DynamicArrayFiles_add(&pending_files, file);
            }
        }

        DynamicArrayFiles_free(&g_files_to_process);
        g_files_to_process = pending_files;

        if (!config.quiet_mode) {
            printf("Resume enabled: %zu files restored from cache, %zu files need processing.\n",
                   cached_hits, g_files_to_process.count);
        }

        DynamicArrayCacheEntries_free(&cache_entries);
    }

    size_t planned_jobs = config.max_concurrent_tasks > 0
                          ? (size_t)config.max_concurrent_tasks
                          : detect_default_concurrency();

    if (g_files_to_process.count > 0) {
        if (!config.quiet_mode) {
            printf("Scan done: %zu audio files pending.\n", g_files_to_process.count);
            printf("Using up to %zu concurrent workers.\n", planned_jobs);
        }

        dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        dispatch_group_t group = dispatch_group_create();
        dispatch_semaphore_t throttle = dispatch_semaphore_create((long)planned_jobs);

        for (size_t i = 0; i < g_files_to_process.count; ++i) {
            dispatch_semaphore_wait(throttle, DISPATCH_TIME_FOREVER);

            GCDTaskContext *task_ctx = (GCDTaskContext *)malloc(sizeof(GCDTaskContext));
            if (!task_ctx) {
                dispatch_semaphore_signal(throttle);
                ProcessResult fail_result;
                safe_strncpy(fail_result.relative_path,
                             g_files_to_process.items[i].relative_path,
                             sizeof(fail_result.relative_path));
                fail_result.success = 0;
                fail_result.lra = NAN;
                safe_strncpy(fail_result.error_message,
                             "Failed to allocate task context",
                             sizeof(fail_result.error_message));
                DynamicArrayResults_add(&g_processing_results, &fail_result);
                atomic_fetch_add(&g_processed_file_count, 1);
                continue;
            }

            task_ctx->file_index = i;
            task_ctx->files_list_ptr = &g_files_to_process;
            task_ctx->results_list_ptr = &g_processing_results;
            task_ctx->results_mutex_ptr = &g_results_mutex;
            task_ctx->processed_count_ptr = &g_processed_file_count;
            task_ctx->total_files_for_progress = g_files_to_process.count;
            task_ctx->quiet_mode = config.quiet_mode;
            task_ctx->retry_count = config.retry_count;
            task_ctx->ffmpeg_config = config.ffmpeg_config;
            task_ctx->throttle_semaphore = throttle;
            task_ctx->throttle_enabled = 1;

            dispatch_group_async_f(group, queue, task_ctx, process_file_gcd_task_f);
        }

        dispatch_group_wait(group, DISPATCH_TIME_FOREVER);

#if !OS_OBJECT_USE_OBJC
        if (group) dispatch_release(group);
        if (throttle) dispatch_release(throttle);
#endif
    }

    size_t success_count = 0;
    size_t failure_count = 0;
    for (size_t i = 0; i < g_processing_results.count; ++i) {
        if (g_processing_results.items[i].success) {
            success_count++;
        } else {
            failure_count++;
        }
    }

    if (!write_sorted_results_by_format(results_file_path_abs, &g_processing_results,
                                        LRA_HEADER_LINE, config.output_format)) {
        fprintf(stderr, "Error: failed to write results file\n");
    }

    if (config.generate_failure_report && failure_count > 0) {
        if (!write_failure_report(failure_report_path_abs, &g_processing_results)) {
            fprintf(stderr, "Error: failed to write failure report\n");
        } else if (!config.quiet_mode) {
            printf("Failure report written: %s\n", failure_report_path_abs);
        }
    }

    if (config.resume_mode) {
        DynamicArrayCacheEntries new_cache;
        if (DynamicArrayCacheEntries_init(&new_cache, all_scanned_files.count + 8)) {
            for (size_t i = 0; i < all_scanned_files.count; ++i) {
                const FileToProcess *file = &all_scanned_files.items[i];
                const ProcessResult *result =
                    find_success_result_by_relative_path(&g_processing_results, file->relative_path);
                if (!result) continue;

                CacheEntry cache_entry;
                safe_strncpy(cache_entry.relative_path, file->relative_path,
                             sizeof(cache_entry.relative_path));
                cache_entry.file_size_bytes = file->file_size_bytes;
                cache_entry.modified_time_epoch = file->modified_time_epoch;
                cache_entry.lra = result->lra;
                upsert_cache_entry(&new_cache, &cache_entry);
            }

            if (!save_cache_file(cache_file_path_abs, &new_cache)) {
                fprintf(stderr, "Warning: failed to save cache file\n");
            } else if (config.verbose_mode && !config.quiet_mode) {
                printf("Cache file updated: %s\n", cache_file_path_abs);
            }
            DynamicArrayCacheEntries_free(&new_cache);
        }
    }

    if (!config.quiet_mode) {
        printf("Success: %zu, Failed: %zu\n", success_count, failure_count);
        printf("Results written: %s\n", results_file_path_abs);
    }

    DynamicArrayFiles_free(&all_scanned_files);
    DynamicArrayFiles_free(&g_files_to_process);
    DynamicArrayResults_free(&g_processing_results);
    pthread_mutex_destroy(&g_results_mutex);

    return 0;
}
