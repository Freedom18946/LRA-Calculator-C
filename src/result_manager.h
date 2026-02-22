#ifndef RESULT_MANAGER_H
#define RESULT_MANAGER_H

#include "utils.h"

typedef enum {
    OUTPUT_FORMAT_TEXT = 0,
    OUTPUT_FORMAT_CSV = 1,
    OUTPUT_FORMAT_JSON = 2
} OutputFormat;

const char* output_format_to_extension(OutputFormat format);
int write_results_to_file(const char *results_file_path, const DynamicArrayResults *results, const char* header);
int sort_results_file(const char *results_file_path, const char* header);
int write_sorted_results_by_format(const char *results_file_path, const DynamicArrayResults *results,
                                   const char *header, OutputFormat format);
int write_failure_report(const char *failure_report_path, const DynamicArrayResults *results);

#endif // RESULT_MANAGER_H
