#include "ffmpeg_processor.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void apply_resource_limits(const FFmpegExecutionConfig *config) {
    if (!config) return;

    if (config->max_cpu_seconds > 0) {
        struct rlimit cpu_limit;
        cpu_limit.rlim_cur = (rlim_t)config->max_cpu_seconds;
        cpu_limit.rlim_max = (rlim_t)config->max_cpu_seconds;
        (void)setrlimit(RLIMIT_CPU, &cpu_limit);
    }

    if (config->max_memory_mb > 0) {
        rlim_t mem_bytes = (rlim_t)config->max_memory_mb * 1024ULL * 1024ULL;
        struct rlimit as_limit;
        as_limit.rlim_cur = mem_bytes;
        as_limit.rlim_max = mem_bytes;
        (void)setrlimit(RLIMIT_AS, &as_limit);
    }
}

static void update_lra_from_line(const char *line, ProcessResult *result, int *found_lra,
                                 char *last_meaningful_line, size_t last_line_size) {
    if (!line || !result || !found_lra || !last_meaningful_line) return;

    if (line[0] != '\0') {
        safe_strncpy(last_meaningful_line, line, last_line_size);
    }

    const char *lra_ptr = strstr(line, "LRA:");
    if (!lra_ptr) return;

    lra_ptr += strlen("LRA:");
    while (*lra_ptr && isspace((unsigned char)*lra_ptr)) {
        lra_ptr++;
    }

    double parsed_lra = NAN;
    if (sscanf(lra_ptr, "%lf", &parsed_lra) == 1) {
        result->lra = parsed_lra;
        *found_lra = 1;
    }
}

static int parse_ffmpeg_stream(int read_fd, pid_t child_pid, ProcessResult *result,
                               const FFmpegExecutionConfig *config, int *child_status,
                               char *last_meaningful_line, size_t last_line_size,
                               int *timed_out) {
    char chunk[1024];
    char line_buffer[MAX_LINE_LEN];
    size_t line_len = 0;
    int found_lra = 0;
    int pipe_eof = 0;
    int child_exited = 0;
    time_t start_time = time(NULL);
    int timeout_seconds = (config && config->timeout_seconds > 0) ? config->timeout_seconds : 120;

    if (child_status) *child_status = 0;
    if (timed_out) *timed_out = 0;

    while (!pipe_eof || !child_exited) {
        if (!child_exited) {
            pid_t wait_result = waitpid(child_pid, child_status, WNOHANG);
            if (wait_result == child_pid) {
                child_exited = 1;
            }
        }

        if (!child_exited && timeout_seconds > 0 && (time(NULL) - start_time) > timeout_seconds) {
            if (timed_out) *timed_out = 1;
            kill(child_pid, SIGKILL);
            waitpid(child_pid, child_status, 0);
            child_exited = 1;
        }

        if (pipe_eof) {
            if (child_exited) break;
            usleep(20 * 1000);
            continue;
        }

        struct pollfd pfd;
        pfd.fd = read_fd;
        pfd.events = POLLIN;
        int poll_result = poll(&pfd, 1, 100);

        if (poll_result < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (poll_result == 0) continue;

        if (pfd.revents & (POLLIN | POLLHUP)) {
            ssize_t bytes_read = read(read_fd, chunk, sizeof(chunk));
            if (bytes_read == 0) {
                pipe_eof = 1;
                continue;
            }
            if (bytes_read < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;
                break;
            }

            for (ssize_t i = 0; i < bytes_read; ++i) {
                char ch = chunk[i];
                if (ch == '\r') continue;

                if (ch == '\n') {
                    line_buffer[line_len] = '\0';
                    update_lra_from_line(line_buffer, result, &found_lra,
                                         last_meaningful_line, last_line_size);
                    line_len = 0;
                } else if (line_len < sizeof(line_buffer) - 1) {
                    line_buffer[line_len++] = ch;
                } else {
                    line_buffer[line_len] = '\0';
                    update_lra_from_line(line_buffer, result, &found_lra,
                                         last_meaningful_line, last_line_size);
                    line_len = 0;
                }
            }
        }
    }

    if (line_len > 0) {
        line_buffer[line_len] = '\0';
        update_lra_from_line(line_buffer, result, &found_lra,
                             last_meaningful_line, last_line_size);
    }

    return found_lra;
}

void calculate_lra_for_file(const FileToProcess* file_task, ProcessResult* result,
                            const FFmpegExecutionConfig *config) {
    if (!file_task || !result) {
        if (result) {
            result->success = 0;
            safe_strncpy(result->error_message,
                         "Invalid parameters passed to calculate_lra_for_file",
                         MAX_LINE_LEN);
        }
        return;
    }

    safe_strncpy(result->relative_path, file_task->relative_path, MAX_PATH_LEN);
    result->lra = NAN;
    result->success = 0;
    result->error_message[0] = '\0';

    int pipe_fd[2];
    if (pipe(pipe_fd) != 0) {
        snprintf(result->error_message, MAX_LINE_LEN,
                 "Failed to create pipe for ffmpeg: %s", strerror(errno));
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        snprintf(result->error_message, MAX_LINE_LEN,
                 "Failed to fork ffmpeg process: %s", strerror(errno));
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return;
    }

    if (pid == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDERR_FILENO);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);

        apply_resource_limits(config);

        char arg0[] = "ffmpeg";
        char arg1[] = "-nostdin";
        char arg2[] = "-i";
        char input_path[MAX_PATH_LEN];
        safe_strncpy(input_path, file_task->full_path, sizeof(input_path));
        char arg4[] = "-filter_complex";
        char arg5[] = "ebur128";
        char arg6[] = "-f";
        char arg7[] = "null";
        char arg8[] = "-hide_banner";
        char arg9[] = "-loglevel";
        char arg10[] = "info";
        char arg11[] = "-";

        char *argv[] = {
            arg0,
            arg1,
            arg2,
            input_path,
            arg4,
            arg5,
            arg6,
            arg7,
            arg8,
            arg9,
            arg10,
            arg11,
            NULL
        };

        execvp(arg0, argv);
        _exit(127);
    }

    close(pipe_fd[1]);

    int flags = fcntl(pipe_fd[0], F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(pipe_fd[0], F_SETFL, flags | O_NONBLOCK);
    }

    char last_meaningful_line[MAX_LINE_LEN] = "N/A";
    int child_status = 0;
    int timed_out = 0;

    int found_lra = parse_ffmpeg_stream(pipe_fd[0], pid, result, config,
                                        &child_status, last_meaningful_line,
                                        sizeof(last_meaningful_line), &timed_out);
    close(pipe_fd[0]);

    if (timed_out) {
        snprintf(result->error_message, MAX_LINE_LEN,
                 "ffmpeg timed out for %s", file_task->full_path);
        result->success = 0;
        return;
    }

    if (found_lra) {
        result->success = 1;
        return;
    }

    if (WIFEXITED(child_status)) {
        int exit_status = WEXITSTATUS(child_status);
        snprintf(result->error_message, MAX_LINE_LEN,
                 "ffmpeg exited with error code %d for %s. Last output: %s",
                 exit_status, file_task->full_path, last_meaningful_line);
    } else if (WIFSIGNALED(child_status)) {
        snprintf(result->error_message, MAX_LINE_LEN,
                 "ffmpeg terminated by signal %d for %s. Last output: %s",
                 WTERMSIG(child_status), file_task->full_path,
                 last_meaningful_line);
    } else {
        snprintf(result->error_message, MAX_LINE_LEN,
                 "Could not parse LRA from ffmpeg output for %s. Last output: %s",
                 file_task->full_path, last_meaningful_line);
    }

    result->success = 0;
}
