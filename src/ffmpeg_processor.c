#include "ffmpeg_processor.h"
#include <stdio.h>
#include <errno.h>     // For strerror
#include <sys/wait.h>  // For WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG

/**
 * 计算指定音频文件的响度范围 (LRA) / Calculate Loudness Range (LRA) for specified audio file
 *
 * 此函数使用 FFmpeg 的 ebur128 滤镜来分析音频文件并计算 LRA 值。
 * This function uses FFmpeg's ebur128 filter to analyze audio files and calculate LRA values.
 *
 * @param file_task 包含文件路径信息的结构体指针 / Pointer to structure containing file path information
 * @param result 用于存储计算结果的结构体指针 / Pointer to structure for storing calculation results
 */
void calculate_lra_for_file(const FileToProcess* file_task, ProcessResult* result) {
    // 参数验证 / Parameter validation
    if (!file_task || !result) {
        if (result) {
            result->success = 0;
            safe_strncpy(result->error_message, "Invalid parameters passed to calculate_lra_for_file", MAX_LINE_LEN);
        }
        return;
    }

    char command[MAX_CMD_LEN];
    FILE *fp = NULL;
    char line_buffer[MAX_LINE_LEN];
    int found_lra = 0;

    // 初始化结果结构体 / Initialize result structure
    safe_strncpy(result->relative_path, file_task->relative_path, MAX_PATH_LEN);
    result->lra = NAN;
    result->success = 0;
    result->error_message[0] = '\0';

    // 构建 FFmpeg 命令 / Build FFmpeg command
    // 使用 ebur128 滤镜进行响度分析 / Use ebur128 filter for loudness analysis
    int cmd_len = snprintf(command, sizeof(command),
                          "ffmpeg -nostdin -i \"%s\" -filter_complex ebur128 -f null -hide_banner -loglevel info - 2>&1",
                          file_task->full_path);

    // 检查命令长度是否超出缓冲区 / Check if command length exceeds buffer
    if (cmd_len >= (int)sizeof(command)) {
        snprintf(result->error_message, MAX_LINE_LEN,
                "FFmpeg command too long for file: %s", file_task->relative_path);
        return;
    }

    // 执行 FFmpeg 命令 / Execute FFmpeg command
    fp = popen(command, "r");
    if (fp == NULL) {
        snprintf(result->error_message, MAX_LINE_LEN,
                "Failed to run ffmpeg command for %s. Error: %s",
                file_task->relative_path, strerror(errno));
        return;
    }

    // Clear previous error messages for the same line buffer to avoid confusion
    line_buffer[0] = '\0';
    char last_meaningful_line[MAX_LINE_LEN] = "N/A";

    while (fgets(line_buffer, sizeof(line_buffer), fp) != NULL) {
        if(strlen(line_buffer) > 1) { // Avoid empty lines or only newline
            safe_strncpy(last_meaningful_line, line_buffer, MAX_LINE_LEN);
        }
        char *lra_ptr = strstr(line_buffer, "LRA:");
        if (lra_ptr) {
            lra_ptr += strlen("LRA:");
            while (*lra_ptr && isspace((unsigned char)*lra_ptr)) {
                lra_ptr++;
            }
            if (sscanf(lra_ptr, "%lf", &result->lra) == 1) {
                found_lra = 1;
                // We could break here if LRA is the only thing parsed from summary.
                // However, ffmpeg might print multiple summaries or LRA values in some cases (e.g. multi-stream).
                // The Rust version uses `captures_iter(&stderr_output).last()`, so we take the last one found.
                // Here, we just take the first one that successfully parses. For ebur128, usually one summary block.
                // If multiple LRAs are possible, logic here might need adjustment to pick the "correct" one or last one.
            }
        }
    }

    int status = pclose(fp);
    if (status == -1) {
        if (result->error_message[0] == '\0') { // Only set if not already set by popen failure
            snprintf(result->error_message, MAX_LINE_LEN, "pclose failed for %s. Error: %s", file_task->full_path, strerror(errno));
        }
        if(!found_lra) result->success = 0;
    } else if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status != 0 && !found_lra) {
            if (result->error_message[0] == '\0') {
                snprintf(result->error_message, MAX_LINE_LEN, "ffmpeg exited with error code %d for %s. Last output: %s", exit_status, file_task->full_path, last_meaningful_line);
            }
        }
    } else if (WIFSIGNALED(status)) {
        if (!found_lra && result->error_message[0] == '\0') {
            snprintf(result->error_message, MAX_LINE_LEN, "ffmpeg terminated by signal %d for %s. Last output: %s", WTERMSIG(status), file_task->full_path, last_meaningful_line);
        }
    }


    if (found_lra) {
        result->success = 1;
    } else {
        if (result->error_message[0] == '\0') {
            snprintf(result->error_message, MAX_LINE_LEN, "Could not parse LRA from ffmpeg output for %s. Last output: %s", file_task->full_path, last_meaningful_line);
        }
        result->success = 0;
    }
}