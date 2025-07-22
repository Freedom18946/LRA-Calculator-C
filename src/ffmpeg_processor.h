#ifndef FFMPEG_PROCESSOR_H
#define FFMPEG_PROCESSOR_H

#include "utils.h"

void calculate_lra_for_file(const FileToProcess* file_task, ProcessResult* result);

#endif // FFMPEG_PROCESSOR_H