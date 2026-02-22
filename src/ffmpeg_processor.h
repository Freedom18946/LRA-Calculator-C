#ifndef FFMPEG_PROCESSOR_H
#define FFMPEG_PROCESSOR_H

#include "utils.h"

typedef struct {
    int timeout_seconds;
    int max_cpu_seconds;
    size_t max_memory_mb;
} FFmpegExecutionConfig;

void calculate_lra_for_file(const FileToProcess* file_task, ProcessResult* result,
                            const FFmpegExecutionConfig *config);

#endif // FFMPEG_PROCESSOR_H
