#ifndef RESULT_MANAGER_H
#define RESULT_MANAGER_H

#include "utils.h"

int write_results_to_file(const char *results_file_path, const DynamicArrayResults *results, const char* header);
int sort_results_file(const char *results_file_path, const char* header);

#endif // RESULT_MANAGER_H