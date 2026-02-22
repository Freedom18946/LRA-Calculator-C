#ifndef CACHE_MANAGER_H
#define CACHE_MANAGER_H

#include "utils.h"

typedef struct {
    char relative_path[MAX_PATH_LEN];
    time_t modified_time_epoch;
    long long file_size_bytes;
    double lra;
} CacheEntry;

typedef struct {
    CacheEntry *items;
    size_t count;
    size_t capacity;
} DynamicArrayCacheEntries;

int DynamicArrayCacheEntries_init(DynamicArrayCacheEntries *arr, size_t initial_capacity);
int DynamicArrayCacheEntries_add(DynamicArrayCacheEntries *arr, const CacheEntry *item);
void DynamicArrayCacheEntries_free(DynamicArrayCacheEntries *arr);

const CacheEntry* find_cache_entry(const DynamicArrayCacheEntries *cache_entries,
                                   const FileToProcess *file_info);
int upsert_cache_entry(DynamicArrayCacheEntries *cache_entries, const CacheEntry *entry);
int load_cache_file(const char *cache_file_path, DynamicArrayCacheEntries *cache_entries);
int save_cache_file(const char *cache_file_path, const DynamicArrayCacheEntries *cache_entries);

#endif // CACHE_MANAGER_H
