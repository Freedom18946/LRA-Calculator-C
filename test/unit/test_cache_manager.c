#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cache_manager.h"

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Test failed: %s\n", message); \
            fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__); \
            exit(1); \
        } else { \
            printf("✓ %s\n", message); \
        } \
    } while (0)

static void test_cache_roundtrip() {
    const char *cache_file = "/tmp/lra_cache_roundtrip.tsv";

    DynamicArrayCacheEntries cache_entries;
    TEST_ASSERT(DynamicArrayCacheEntries_init(&cache_entries, 4) == 1, "Init cache entries");

    CacheEntry entry = {
        .relative_path = "a/b.wav",
        .modified_time_epoch = 12345,
        .file_size_bytes = 4567,
        .lra = 8.8
    };

    TEST_ASSERT(upsert_cache_entry(&cache_entries, &entry) == 1, "Insert cache entry");
    TEST_ASSERT(save_cache_file(cache_file, &cache_entries) == 1, "Save cache file");

    DynamicArrayCacheEntries loaded_entries;
    TEST_ASSERT(DynamicArrayCacheEntries_init(&loaded_entries, 4) == 1, "Init loaded cache entries");
    TEST_ASSERT(load_cache_file(cache_file, &loaded_entries) == 1, "Load cache file");

    FileToProcess file = {
        .full_path = "/tmp/a/b.wav",
        .relative_path = "a/b.wav",
        .file_size_bytes = 4567,
        .modified_time_epoch = 12345
    };
    const CacheEntry *loaded = find_cache_entry(&loaded_entries, &file);
    TEST_ASSERT(loaded != NULL, "Find cache entry by metadata");
    TEST_ASSERT(strcmp(loaded->relative_path, "a/b.wav") == 0, "Loaded cache path matches");
    TEST_ASSERT((int)(loaded->lra * 10) == 88, "Loaded cache LRA matches");

    DynamicArrayCacheEntries_free(&cache_entries);
    DynamicArrayCacheEntries_free(&loaded_entries);
    unlink(cache_file);
}

int main() {
    printf("Running cache_manager unit tests...\n");
    test_cache_roundtrip();
    printf("All cache_manager tests passed.\n");
    return 0;
}
