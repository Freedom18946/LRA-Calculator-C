#ifndef FILE_SCANNER_H
#define FILE_SCANNER_H

#include "utils.h"
#include <ftw.h> // For nftw related type FTW

typedef struct {
    const char *include_pattern;
    const char *exclude_pattern;
} ScanOptions;

// Context data for the nftw callback
typedef struct {
    DynamicArrayFiles *files_list;
    const char *base_path_in;
    const char *results_filename_abs;
    const ScanOptions *scan_options;
} NFTWContext;

// Scans directory using nftw and populates the files_list
int scan_files_with_nftw(const char *dir_path, DynamicArrayFiles *files_list,
                         const char *base_path, const char *results_filename_abs,
                         const ScanOptions *scan_options);

#endif // FILE_SCANNER_H
