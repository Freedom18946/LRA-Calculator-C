/**
 * 工具函数头文件 / Utility Functions Header File
 *
 * 本文件定义了 LRA-Calculator-C 项目中使用的各种工具函数、数据结构和常量。
 * 包括字符串处理、文件路径操作、动态数组管理等核心功能。
 *
 * This file defines various utility functions, data structures, and constants
 * used in the LRA-Calculator-C project. Includes string processing, file path
 * operations, dynamic array management, and other core functionalities.
 *
 * 作者 / Author: LRA-Calculator-C Development Team
 * 版本 / Version: 2.0
 * 日期 / Date: 2025-08-07
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>  // 时间函数 / Time functions
#include <ctype.h> // 字符处理函数 (如 tolower) / Character processing functions (like tolower)
#include <math.h>  // 数学函数 (如 NAN, isnan) / Math functions (like NAN, isnan)
#include <float.h> // 浮点数常量 (如 DBL_MAX) / Floating point constants (like DBL_MAX)

// === 常量定义 / Constant Definitions ===

#define MAX_PATH_LEN 4096              // 最大路径长度 / Maximum path length
#define MAX_LINE_LEN 4096              // 最大行长度 / Maximum line length
#define MAX_CMD_LEN 5120               // 最大命令长度 / Maximum command length
#define LRA_RESULTS_FILENAME "lra_results.txt"  // 结果文件名 / Result file name
#define LRA_HEADER_LINE "文件路径 (相对) - LRA 数值 (LU)"  // 结果文件头部行 / Result file header line

#define NUM_SUPPORTED_EXTENSIONS 10    // 支持的音频格式数量 / Number of supported audio formats

/**
 * 支持的音频文件扩展名数组 / Array of supported audio file extensions
 * 包含常见的音频格式如 WAV, MP3, FLAC, AAC 等
 * Contains common audio formats like WAV, MP3, FLAC, AAC, etc.
 */
extern const char *SUPPORTED_EXTENSIONS[NUM_SUPPORTED_EXTENSIONS];

// === 数据结构定义 / Data Structure Definitions ===

/**
 * 待处理文件信息结构体 / File to Process Information Structure
 *
 * 存储单个音频文件的路径信息，包括完整路径和相对路径。
 * 相对路径用于结果显示，完整路径用于实际文件访问。
 *
 * Stores path information for a single audio file, including full path and relative path.
 * Relative path is used for result display, full path is used for actual file access.
 */
typedef struct {
    char full_path[MAX_PATH_LEN];     // 文件的完整绝对路径 / Full absolute path of the file
    char relative_path[MAX_PATH_LEN]; // 相对于基础目录的路径 / Path relative to base directory
    long long file_size_bytes;        // 文件大小 (字节) / File size in bytes
    time_t modified_time_epoch;       // 修改时间 (epoch) / Modification time (epoch)
} FileToProcess;

/**
 * 文件列表动态数组结构体 / Dynamic Array Structure for File List
 *
 * 用于存储待处理文件列表的动态数组，支持自动扩容。
 * 在文件扫描阶段填充，在并行处理阶段读取。
 *
 * Dynamic array for storing list of files to process, supports automatic expansion.
 * Populated during file scanning phase, read during parallel processing phase.
 */
typedef struct {
    FileToProcess *items;  // 文件信息数组指针 / Pointer to array of file information
    size_t count;          // 当前文件数量 / Current number of files
    size_t capacity;       // 数组容量 / Array capacity
} DynamicArrayFiles;

/**
 * 处理结果结构体 / Processing Result Structure
 *
 * 存储单个文件的 LRA 计算结果，包括成功状态、LRA 值和错误信息。
 * 用于在并发处理过程中收集和管理结果。
 *
 * Stores LRA calculation result for a single file, including success status,
 * LRA value, and error information. Used to collect and manage results during concurrent processing.
 */
typedef struct {
    char relative_path[MAX_PATH_LEN]; // 文件相对路径 / File relative path
    double lra;                       // LRA 值 (响度范围，单位 LU) / LRA value (Loudness Range in LU)
    int success;                      // 处理成功标志 (1=成功, 0=失败) / Processing success flag (1=success, 0=failure)
    char error_message[MAX_LINE_LEN]; // 错误信息 (失败时使用) / Error message (used when failed)
} ProcessResult;

/**
 * 结果列表动态数组结构体 / Dynamic Array Structure for Result List
 *
 * 用于存储处理结果的动态数组，支持线程安全的并发访问。
 * 在并行处理阶段由多个工作线程写入，最后统一输出到文件。
 *
 * Dynamic array for storing processing results, supports thread-safe concurrent access.
 * Written by multiple worker threads during parallel processing phase, finally output to file uniformly.
 */
typedef struct {
    ProcessResult *items; // 结果数组指针 / Pointer to array of results
    size_t count;         // 当前结果数量 / Current number of results
    size_t capacity;      // 数组容量 / Array capacity
} DynamicArrayResults;


void get_current_time_str(char *buffer, size_t buffer_size);
const char *get_filename_ext(const char *filename);
void string_to_lower(char *str);
int is_supported_extension(const char *filename);
char* get_relative_path(const char *base_path, const char *full_path, char *output_relative_path, size_t output_size);

// DynamicArrayFiles functions
int DynamicArrayFiles_init(DynamicArrayFiles *arr, size_t initial_capacity);
int DynamicArrayFiles_add(DynamicArrayFiles *arr, const FileToProcess *item);
void DynamicArrayFiles_free(DynamicArrayFiles *arr);

// DynamicArrayResults functions
int DynamicArrayResults_init(DynamicArrayResults *arr, size_t initial_capacity);
int DynamicArrayResults_add(DynamicArrayResults *arr, const ProcessResult *item);
void DynamicArrayResults_free(DynamicArrayResults *arr);

// A safer strncpy that ensures null termination
char *safe_strncpy(char *dest, const char *src, size_t n);

#endif // UTILS_H
