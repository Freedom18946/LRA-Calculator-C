#include "result_manager.h"
#include <stdio.h>
#include <errno.h> // For strerror

int write_results_to_file(const char *results_file_path, const DynamicArrayResults *results, const char* header) {
    FILE *fp = fopen(results_file_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open results file %s for writing: %s\n", results_file_path, strerror(errno));
        return 0;
    }

    fprintf(fp, "%s\n", header);
    for (size_t i = 0; i < results->count; ++i) {
        if (results->items[i].success) {
            fprintf(fp, "%s - %.1f\n", results->items[i].relative_path, results->items[i].lra);
        }
    }

    fclose(fp);
    return 1;
}


typedef struct {
    char path_part[MAX_PATH_LEN];
    double lra_value;
} SortEntry;

static int compare_sort_entries(const void *a, const void *b) {
    const SortEntry *entry_a = (const SortEntry *)a;
    const SortEntry *entry_b = (const SortEntry *)b;

    if (entry_a->lra_value < entry_b->lra_value) return 1;
    if (entry_a->lra_value > entry_b->lra_value) return -1;
    return 0;
}

int sort_results_file(const char *results_file_path, const char* header) {
    FILE *fp_read = fopen(results_file_path, "r");
    if (!fp_read) {
        fprintf(stderr, "Error: Cannot open results file %s for reading (sorting): %s\n", results_file_path, strerror(errno));
        return 0;
    }

    SortEntry* sort_entries_arr = (SortEntry*)malloc(64 * sizeof(SortEntry));
    if (!sort_entries_arr) {
        perror("Failed to allocate memory for sort_entries_arr");
        fclose(fp_read);
        return 0;
    }
    size_t sort_entries_count = 0;
    size_t sort_entries_capacity = 64;


    char line[MAX_LINE_LEN];
    if (fgets(line, sizeof(line), fp_read) == NULL && !feof(fp_read)) {
        // File might be empty or just header failed to read
    }


    while (fgets(line, sizeof(line), fp_read) != NULL) {
        if (strlen(line) > 0 && line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }
        if (strlen(line) == 0) continue;

        char *separator = strstr(line, " - ");
        if (separator) {
            SortEntry current_entry;
            size_t path_len = separator - line;
            if (path_len >= MAX_PATH_LEN) path_len = MAX_PATH_LEN - 1;
            strncpy(current_entry.path_part, line, path_len);
            current_entry.path_part[path_len] = '\0';

            char* lra_str_part = separator + strlen(" - ");
            if (sscanf(lra_str_part, "%lf", &current_entry.lra_value) == 1) {
                if (sort_entries_count >= sort_entries_capacity) {
                    size_t new_capacity = sort_entries_capacity * 2;
                    SortEntry* new_arr = (SortEntry*)realloc(sort_entries_arr, new_capacity * sizeof(SortEntry));
                    if (!new_arr) {
                        perror("Failed to reallocate sort_entries_arr");
                        fclose(fp_read);
                        free(sort_entries_arr);
                        return 0;
                    }
                    sort_entries_arr = new_arr;
                    sort_entries_capacity = new_capacity;
                }
                sort_entries_arr[sort_entries_count++] = current_entry;
            } else {
                fprintf(stderr, "Sorting warning: Could not parse LRA value from line: %s\n", line);
            }
        } else {
            fprintf(stderr, "Sorting warning: Line format incorrect: %s\n", line);
        }
    }
    fclose(fp_read);

    if (sort_entries_count == 0) {
        printf("No entries to sort in %s.\n", results_file_path);
        FILE *fp_write_header = fopen(results_file_path, "w"); // Re-write header if file became empty
        if (!fp_write_header) {
            fprintf(stderr, "Error: Cannot open results file %s to write header: %s\n", results_file_path, strerror(errno));
            free(sort_entries_arr);
            return 0;
        }
        fprintf(fp_write_header, "%s\n", header);
        fclose(fp_write_header);
        free(sort_entries_arr);
        return 1;
    }


    qsort(sort_entries_arr, sort_entries_count, sizeof(SortEntry), compare_sort_entries);

    FILE *fp_write = fopen(results_file_path, "w");
    if (!fp_write) {
        fprintf(stderr, "Error: Cannot open results file %s for writing sorted data: %s\n", results_file_path, strerror(errno));
        free(sort_entries_arr);
        return 0;
    }

    fprintf(fp_write, "%s\n", header);
    for (size_t i = 0; i < sort_entries_count; ++i) {
        fprintf(fp_write, "%s - %.1f\n", sort_entries_arr[i].path_part, sort_entries_arr[i].lra_value);
    }
    fclose(fp_write);
    free(sort_entries_arr);

    printf("Results file %s sorted successfully.\n", results_file_path);
    return 1;
}