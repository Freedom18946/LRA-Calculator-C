#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // For time functions
#include <ctype.h> // For tolower
#include <math.h>  // For NAN, isnan
#include <float.h> // For DBL_MAX

#define MAX_PATH_LEN 4096
#define MAX_LINE_LEN 4096
#define MAX_CMD_LEN 5120
#define LRA_RESULTS_FILENAME "lra_results.txt"
#define LRA_HEADER_LINE "文件路径 (相对) - LRA 数值 (LU)"

#define NUM_SUPPORTED_EXTENSIONS 10
extern const char *SUPPORTED_EXTENSIONS[NUM_SUPPORTED_EXTENSIONS];

// Structure for storing file paths to process
typedef struct {
    char full_path[MAX_PATH_LEN];
    char relative_path[MAX_PATH_LEN];
} FileToProcess;

// Dynamic array for FileToProcess
typedef struct {
    FileToProcess *items;
    size_t count;
    size_t capacity;
} DynamicArrayFiles;

// Structure for storing processing results
typedef struct {
    char relative_path[MAX_PATH_LEN];
    double lra;
    int success; // 1 for success, 0 for failure
    char error_message[MAX_LINE_LEN];
} ProcessResult;

// Dynamic array for ProcessResult
typedef struct {
    ProcessResult *items;
    size_t count;
    size_t capacity;
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