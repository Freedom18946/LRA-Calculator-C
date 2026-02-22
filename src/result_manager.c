#include "result_manager.h"

#include <errno.h>


typedef struct {
    char path_part[MAX_PATH_LEN];
    double lra_value;
} SortEntry;

static int compare_sort_entries(const void *a, const void *b) {
    const SortEntry *entry_a = (const SortEntry *)a;
    const SortEntry *entry_b = (const SortEntry *)b;

    if (entry_a->lra_value < entry_b->lra_value) return 1;
    if (entry_a->lra_value > entry_b->lra_value) return -1;
    return strcmp(entry_a->path_part, entry_b->path_part);
}

static const char *find_last_separator(const char *line, const char *separator) {
    const char *cursor = line;
    const char *last = NULL;
    while ((cursor = strstr(cursor, separator)) != NULL) {
        last = cursor;
        cursor += 1;
    }
    return last;
}

static int collect_success_entries(const DynamicArrayResults *results,
                                   SortEntry **entries_out,
                                   size_t *count_out,
                                   size_t *capacity_out) {
    if (!results || !entries_out || !count_out || !capacity_out) return 0;

    size_t capacity = results->count > 0 ? results->count : 1;
    SortEntry *entries = (SortEntry *)malloc(capacity * sizeof(SortEntry));
    if (!entries) return 0;

    size_t count = 0;
    for (size_t i = 0; i < results->count; ++i) {
        if (!results->items[i].success) continue;
        safe_strncpy(entries[count].path_part, results->items[i].relative_path,
                     sizeof(entries[count].path_part));
        entries[count].lra_value = results->items[i].lra;
        count++;
    }

    qsort(entries, count, sizeof(SortEntry), compare_sort_entries);

    *entries_out = entries;
    *count_out = count;
    *capacity_out = capacity;
    return 1;
}

static void write_json_escaped_string(FILE *fp, const char *text) {
    fputc('"', fp);
    for (const char *c = text; *c; ++c) {
        switch (*c) {
            case '\\': fputs("\\\\", fp); break;
            case '"': fputs("\\\"", fp); break;
            case '\n': fputs("\\n", fp); break;
            case '\r': fputs("\\r", fp); break;
            case '\t': fputs("\\t", fp); break;
            default: fputc(*c, fp); break;
        }
    }
    fputc('"', fp);
}

static void write_csv_escaped_string(FILE *fp, const char *text) {
    fputc('"', fp);
    for (const char *c = text; *c; ++c) {
        if (*c == '"') {
            fputs("\"\"", fp);
        } else {
            fputc(*c, fp);
        }
    }
    fputc('"', fp);
}

const char* output_format_to_extension(OutputFormat format) {
    switch (format) {
        case OUTPUT_FORMAT_CSV:
            return "csv";
        case OUTPUT_FORMAT_JSON:
            return "json";
        case OUTPUT_FORMAT_TEXT:
        default:
            return "txt";
    }
}

int write_results_to_file(const char *results_file_path, const DynamicArrayResults *results,
                          const char* header) {
    FILE *fp = fopen(results_file_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open results file %s for writing: %s\n",
                results_file_path, strerror(errno));
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

int write_sorted_results_by_format(const char *results_file_path,
                                   const DynamicArrayResults *results,
                                   const char *header,
                                   OutputFormat format) {
    SortEntry *entries = NULL;
    size_t count = 0;
    size_t capacity = 0;

    if (!collect_success_entries(results, &entries, &count, &capacity)) {
        fprintf(stderr, "Error: Failed to allocate memory for sorted output entries.\n");
        return 0;
    }
    (void)capacity;

    FILE *fp = fopen(results_file_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open results file %s for writing: %s\n",
                results_file_path, strerror(errno));
        free(entries);
        return 0;
    }

    if (format == OUTPUT_FORMAT_CSV) {
        fprintf(fp, "relative_path,lra\n");
        for (size_t i = 0; i < count; ++i) {
            write_csv_escaped_string(fp, entries[i].path_part);
            fprintf(fp, ",%.1f\n", entries[i].lra_value);
        }
    } else if (format == OUTPUT_FORMAT_JSON) {
        fprintf(fp, "[\n");
        for (size_t i = 0; i < count; ++i) {
            fprintf(fp, "  {\"relative_path\": ");
            write_json_escaped_string(fp, entries[i].path_part);
            fprintf(fp, ", \"lra\": %.1f}%s\n", entries[i].lra_value,
                    (i + 1 < count) ? "," : "");
        }
        fprintf(fp, "]\n");
    } else {
        fprintf(fp, "%s\n", header);
        for (size_t i = 0; i < count; ++i) {
            fprintf(fp, "%s - %.1f\n", entries[i].path_part, entries[i].lra_value);
        }
    }

    fclose(fp);
    free(entries);
    return 1;
}

int write_failure_report(const char *failure_report_path, const DynamicArrayResults *results) {
    if (!failure_report_path || !results) return 0;

    FILE *fp = fopen(failure_report_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open failure report %s for writing: %s\n",
                failure_report_path, strerror(errno));
        return 0;
    }

    fprintf(fp, "失败文件报告 / Failed Files Report\n");
    fprintf(fp, "----------------------------------------\n");
    for (size_t i = 0; i < results->count; ++i) {
        if (results->items[i].success) continue;
        fprintf(fp, "%s\t%s\n", results->items[i].relative_path,
                results->items[i].error_message);
    }

    fclose(fp);
    return 1;
}

int sort_results_file(const char *results_file_path, const char* header) {
    FILE *fp_read = fopen(results_file_path, "r");
    if (!fp_read) {
        fprintf(stderr, "Error: Cannot open results file %s for reading (sorting): %s\n",
                results_file_path, strerror(errno));
        return 0;
    }

    SortEntry *sort_entries_arr = (SortEntry *)malloc(64 * sizeof(SortEntry));
    if (!sort_entries_arr) {
        perror("Failed to allocate memory for sort_entries_arr");
        fclose(fp_read);
        return 0;
    }

    size_t sort_entries_count = 0;
    size_t sort_entries_capacity = 64;

    char line[MAX_LINE_LEN];
    (void)fgets(line, sizeof(line), fp_read);

    while (fgets(line, sizeof(line), fp_read) != NULL) {
        size_t line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] == '\n') {
            line[line_len - 1] = '\0';
        }
        if (line[0] == '\0') continue;

        const char *separator = find_last_separator(line, " - ");
        if (!separator) {
            fprintf(stderr, "Sorting warning: Line format incorrect: %s\n", line);
            continue;
        }

        SortEntry entry;
        size_t path_len = (size_t)(separator - line);
        if (path_len >= sizeof(entry.path_part)) {
            path_len = sizeof(entry.path_part) - 1;
        }
        memcpy(entry.path_part, line, path_len);
        entry.path_part[path_len] = '\0';

        const char *lra_part = separator + 3;
        if (sscanf(lra_part, "%lf", &entry.lra_value) != 1) {
            fprintf(stderr, "Sorting warning: Could not parse LRA value from line: %s\n", line);
            continue;
        }

        if (sort_entries_count >= sort_entries_capacity) {
            size_t new_capacity = sort_entries_capacity * 2;
            SortEntry *new_arr = (SortEntry *)realloc(sort_entries_arr,
                                                      new_capacity * sizeof(SortEntry));
            if (!new_arr) {
                perror("Failed to reallocate sort_entries_arr");
                fclose(fp_read);
                free(sort_entries_arr);
                return 0;
            }
            sort_entries_arr = new_arr;
            sort_entries_capacity = new_capacity;
        }
        sort_entries_arr[sort_entries_count++] = entry;
    }
    fclose(fp_read);

    qsort(sort_entries_arr, sort_entries_count, sizeof(SortEntry), compare_sort_entries);

    FILE *fp_write = fopen(results_file_path, "w");
    if (!fp_write) {
        fprintf(stderr, "Error: Cannot open results file %s for writing sorted data: %s\n",
                results_file_path, strerror(errno));
        free(sort_entries_arr);
        return 0;
    }

    fprintf(fp_write, "%s\n", header);
    for (size_t i = 0; i < sort_entries_count; ++i) {
        fprintf(fp_write, "%s - %.1f\n", sort_entries_arr[i].path_part,
                sort_entries_arr[i].lra_value);
    }

    fclose(fp_write);
    free(sort_entries_arr);
    return 1;
}
