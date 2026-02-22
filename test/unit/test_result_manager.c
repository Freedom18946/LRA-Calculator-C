#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "result_manager.h"
#include "utils.h"

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

static int file_contains_text(const char *path, const char *needle) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;

    char line[MAX_LINE_LEN];
    int found = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, needle)) {
            found = 1;
            break;
        }
    }
    fclose(fp);
    return found;
}

static void test_sort_results_file_with_separator_in_path() {
    const char *path = "/tmp/lra_result_manager_sort.txt";
    FILE *fp = fopen(path, "w");
    TEST_ASSERT(fp != NULL, "Create temp sort test file");
    fprintf(fp, "%s\n", LRA_HEADER_LINE);
    fprintf(fp, "a - b.wav - 0.0\n");
    fclose(fp);

    TEST_ASSERT(sort_results_file(path, LRA_HEADER_LINE) == 1,
                "sort_results_file succeeds with separator in path");
    TEST_ASSERT(file_contains_text(path, "a - b.wav - 0.0"),
                "Entry containing separator is preserved after sort");

    unlink(path);
}

static void test_write_sorted_results_formats() {
    DynamicArrayResults results;
    TEST_ASSERT(DynamicArrayResults_init(&results, 4) == 1, "Init results array");

    ProcessResult a = {.relative_path = "z.wav", .lra = 1.0, .success = 1, .error_message = ""};
    ProcessResult b = {.relative_path = "a.wav", .lra = 9.0, .success = 1, .error_message = ""};
    ProcessResult c = {.relative_path = "failed.wav", .lra = NAN, .success = 0, .error_message = "failed"};

    DynamicArrayResults_add(&results, &a);
    DynamicArrayResults_add(&results, &b);
    DynamicArrayResults_add(&results, &c);

    const char *txt = "/tmp/lra_results_test.txt";
    const char *csv = "/tmp/lra_results_test.csv";
    const char *json = "/tmp/lra_results_test.json";

    TEST_ASSERT(write_sorted_results_by_format(txt, &results, LRA_HEADER_LINE, OUTPUT_FORMAT_TEXT) == 1,
                "Write text format");
    TEST_ASSERT(write_sorted_results_by_format(csv, &results, LRA_HEADER_LINE, OUTPUT_FORMAT_CSV) == 1,
                "Write CSV format");
    TEST_ASSERT(write_sorted_results_by_format(json, &results, LRA_HEADER_LINE, OUTPUT_FORMAT_JSON) == 1,
                "Write JSON format");

    TEST_ASSERT(file_contains_text(txt, "a.wav - 9.0"), "Text output sorted desc");
    TEST_ASSERT(file_contains_text(csv, "relative_path,lra"), "CSV output header exists");
    TEST_ASSERT(file_contains_text(json, "\"relative_path\""), "JSON output contains keys");

    unlink(txt);
    unlink(csv);
    unlink(json);
    DynamicArrayResults_free(&results);
}

int main() {
    printf("Running result_manager unit tests...\n");
    test_sort_results_file_with_separator_in_path();
    test_write_sorted_results_formats();
    printf("All result_manager tests passed.\n");
    return 0;
}
