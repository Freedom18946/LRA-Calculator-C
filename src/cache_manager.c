#include "cache_manager.h"
#include <errno.h>

#define CACHE_FILE_HEADER "# LRA_CACHE_V1"

int DynamicArrayCacheEntries_init(DynamicArrayCacheEntries *arr, size_t initial_capacity) {
    if (!arr) return 0;
    if (initial_capacity == 0) initial_capacity = 32;
    arr->items = (CacheEntry *)malloc(initial_capacity * sizeof(CacheEntry));
    if (!arr->items) {
        arr->count = 0;
        arr->capacity = 0;
        return 0;
    }
    arr->count = 0;
    arr->capacity = initial_capacity;
    return 1;
}

int DynamicArrayCacheEntries_add(DynamicArrayCacheEntries *arr, const CacheEntry *item) {
    if (!arr || !item) return 0;
    if (arr->count >= arr->capacity) {
        size_t new_capacity = arr->capacity > 0 ? arr->capacity * 2 : 32;
        CacheEntry *new_items = (CacheEntry *)realloc(arr->items, new_capacity * sizeof(CacheEntry));
        if (!new_items) return 0;
        arr->items = new_items;
        arr->capacity = new_capacity;
    }
    arr->items[arr->count++] = *item;
    return 1;
}

void DynamicArrayCacheEntries_free(DynamicArrayCacheEntries *arr) {
    if (!arr) return;
    if (arr->items) free(arr->items);
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

const CacheEntry* find_cache_entry(const DynamicArrayCacheEntries *cache_entries,
                                   const FileToProcess *file_info) {
    if (!cache_entries || !file_info) return NULL;
    for (size_t i = 0; i < cache_entries->count; ++i) {
        const CacheEntry *entry = &cache_entries->items[i];
        if (strcmp(entry->relative_path, file_info->relative_path) == 0 &&
            entry->modified_time_epoch == file_info->modified_time_epoch &&
            entry->file_size_bytes == file_info->file_size_bytes) {
            return entry;
        }
    }
    return NULL;
}

int upsert_cache_entry(DynamicArrayCacheEntries *cache_entries, const CacheEntry *entry) {
    if (!cache_entries || !entry) return 0;
    for (size_t i = 0; i < cache_entries->count; ++i) {
        if (strcmp(cache_entries->items[i].relative_path, entry->relative_path) == 0) {
            cache_entries->items[i] = *entry;
            return 1;
        }
    }
    return DynamicArrayCacheEntries_add(cache_entries, entry);
}

int load_cache_file(const char *cache_file_path, DynamicArrayCacheEntries *cache_entries) {
    if (!cache_file_path || !cache_entries) return 0;

    FILE *fp = fopen(cache_file_path, "r");
    if (!fp) {
        if (errno == ENOENT) return 1;
        fprintf(stderr, "Warning: Failed to open cache file %s: %s\n",
                cache_file_path, strerror(errno));
        return 0;
    }

    char line[MAX_LINE_LEN];
    int is_first_line = 1;
    while (fgets(line, sizeof(line), fp) != NULL) {
        size_t line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] == '\n') line[line_len - 1] = '\0';
        if (line[0] == '\0') continue;

        if (is_first_line) {
            is_first_line = 0;
            if (strcmp(line, CACHE_FILE_HEADER) == 0) {
                continue;
            }
        }

        CacheEntry entry;
        long long mod_time_value = 0;
        if (sscanf(line, "%4095[^\t]\t%lld\t%lld\t%lf",
                   entry.relative_path, &mod_time_value,
                   &entry.file_size_bytes, &entry.lra) == 4) {
            entry.modified_time_epoch = (time_t)mod_time_value;
            if (!upsert_cache_entry(cache_entries, &entry)) {
                fclose(fp);
                return 0;
            }
        }
    }
    fclose(fp);
    return 1;
}

int save_cache_file(const char *cache_file_path, const DynamicArrayCacheEntries *cache_entries) {
    if (!cache_file_path || !cache_entries) return 0;
    FILE *fp = fopen(cache_file_path, "w");
    if (!fp) {
        fprintf(stderr, "Warning: Failed to write cache file %s: %s\n",
                cache_file_path, strerror(errno));
        return 0;
    }

    fprintf(fp, "%s\n", CACHE_FILE_HEADER);
    for (size_t i = 0; i < cache_entries->count; ++i) {
        const CacheEntry *entry = &cache_entries->items[i];
        fprintf(fp, "%s\t%lld\t%lld\t%.3f\n",
                entry->relative_path,
                (long long)entry->modified_time_epoch,
                entry->file_size_bytes,
                entry->lra);
    }

    fclose(fp);
    return 1;
}
