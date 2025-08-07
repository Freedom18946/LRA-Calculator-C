#include "utils.h"
#include <sys/stat.h>
#include <errno.h>

const char *SUPPORTED_EXTENSIONS[NUM_SUPPORTED_EXTENSIONS] = {
        "wav", "mp3", "m4a", "flac", "aac", "ogg", "opus", "wma", "aiff", "alac"
};

void get_current_time_str(char *buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", t);
}

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

void string_to_lower(char *str) {
    if (!str) return;  // 空指针检查 / Null pointer check
    for (size_t i = 0; str[i]; i++) {
        str[i] = (char)tolower((unsigned char)str[i]);
    }
}

int is_supported_extension(const char *filename) {
    if (!filename) return 0;
    const char *ext_const = get_filename_ext(filename);
    if (!ext_const || *ext_const == '\0') return 0;

    char ext[256];
    safe_strncpy(ext, ext_const, sizeof(ext));
    string_to_lower(ext);

    for (size_t i = 0; i < NUM_SUPPORTED_EXTENSIONS; ++i) {
        if (strcmp(ext, SUPPORTED_EXTENSIONS[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

char* get_relative_path(const char *base_path, const char *full_path, char *output_relative_path, size_t output_size) {
    size_t base_len = strlen(base_path);
    if (strncmp(full_path, base_path, base_len) == 0) {
        const char *relative_part = full_path + base_len;
        if (*relative_part == '/' || *relative_part == '\\') {
            relative_part++;
        }
        safe_strncpy(output_relative_path, relative_part, output_size);
        return output_relative_path;
    }
    safe_strncpy(output_relative_path, full_path, output_size);
    return output_relative_path;
}


char *safe_strncpy(char *dest, const char *src, size_t n) {
    if (n == 0) return dest;
    strncpy(dest, src, n - 1);
    dest[n - 1] = '\0';
    return dest;
}

int DynamicArrayFiles_init(DynamicArrayFiles *arr, size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 8;
    arr->items = (FileToProcess *)malloc(initial_capacity * sizeof(FileToProcess));
    if (!arr->items) {
        perror("Failed to allocate memory for DynamicArrayFiles");
        arr->capacity = 0;
        arr->count = 0;
        return 0;
    }
    arr->capacity = initial_capacity;
    arr->count = 0;
    return 1;
}

int DynamicArrayFiles_add(DynamicArrayFiles *arr, const FileToProcess *item) {
    if (arr->count >= arr->capacity) {
        size_t new_capacity = arr->capacity * 2;
        if (new_capacity == 0) new_capacity = 8;
        FileToProcess *new_items = (FileToProcess *)realloc(arr->items, new_capacity * sizeof(FileToProcess));
        if (!new_items) {
            perror("Failed to reallocate memory for DynamicArrayFiles");
            return 0;
        }
        arr->items = new_items;
        arr->capacity = new_capacity;
    }
    arr->items[arr->count++] = *item;
    return 1;
}

void DynamicArrayFiles_free(DynamicArrayFiles *arr) {
    if (arr && arr->items) {
        free(arr->items);
        arr->items = NULL;
        arr->count = 0;
        arr->capacity = 0;
    }
}

int DynamicArrayResults_init(DynamicArrayResults *arr, size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 8;
    arr->items = (ProcessResult *)malloc(initial_capacity * sizeof(ProcessResult));
    if (!arr->items) {
        perror("Failed to allocate memory for DynamicArrayResults");
        arr->capacity = 0;
        arr->count = 0;
        return 0;
    }
    arr->capacity = initial_capacity;
    arr->count = 0;
    return 1;
}

int DynamicArrayResults_add(DynamicArrayResults *arr, const ProcessResult *item) {
    if (arr->count >= arr->capacity) {
        size_t new_capacity = arr->capacity * 2;
        if (new_capacity == 0) new_capacity = 8;
        ProcessResult *new_items = (ProcessResult *)realloc(arr->items, new_capacity * sizeof(ProcessResult));
        if (!new_items) {
            perror("Failed to reallocate memory for DynamicArrayResults");
            return 0;
        }
        arr->items = new_items;
        arr->capacity = new_capacity;
    }
    arr->items[arr->count++] = *item;
    return 1;
}

void DynamicArrayResults_free(DynamicArrayResults *arr) {
    if (arr && arr->items) {
        free(arr->items);
        arr->items = NULL;
        arr->count = 0;
        arr->capacity = 0;
    }
}