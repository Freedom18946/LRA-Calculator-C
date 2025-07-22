#define _XOPEN_SOURCE 500 // Required for nftw
#include <ftw.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "file_scanner.h" // Includes utils.h

// Global static pointer to context data for nftw_callback.
// This is a common C pattern when callback functions don't support arbitrary user data pointers directly.
static NFTWContext *g_nftw_context = NULL;

// Callback function for nftw
static int nftw_callback(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    // Access context data via the global static pointer
    NFTWContext *context = g_nftw_context;
    if (!context) {
        fprintf(stderr, "Error: NFTW context is NULL in callback.\n");
        return -1; // Stop traversal
    }

    // We are only interested in regular files
    if (typeflag == FTW_F) {
        // Skip the results file itself
        if (strcmp(fpath, context->results_filename_abs) == 0) {
            return 0; // Continue traversal
        }

        // Check if the extension is supported
        // fpath + ftwbuf->base gives the filename part
        const char* filename = fpath + ftwbuf->base;
        if (is_supported_extension(filename)) {
            FileToProcess file_info;
            safe_strncpy(file_info.full_path, fpath, MAX_PATH_LEN);
            get_relative_path(context->base_path_in, fpath, file_info.relative_path, MAX_PATH_LEN);

            if (!DynamicArrayFiles_add(context->files_list, &file_info)) {
                fprintf(stderr, "Error: Failed to add file %s to list during nftw scan.\n", fpath);
                // Optionally, return -1 to stop traversal on error
            }
        }
    }
    return 0; // Continue traversal
}

int scan_files_with_nftw(const char *dir_path, DynamicArrayFiles *files_list, const char *base_path, const char *results_filename_abs) {
    NFTWContext context_data;
    context_data.files_list = files_list;
    context_data.base_path_in = base_path;
    context_data.results_filename_abs = results_filename_abs;

    // Set the global static context pointer
    g_nftw_context = &context_data;

    // Call nftw
    // The third argument is the maximum number of file descriptors nftw will use.
    // FTW_PHYS: A physical walk, does not follow symbolic links. This is usually safer.
    // FTW_MOUNT: Stay on the same filesystem.
    // FTW_DEPTH: Process directory contents before the directory itself.
    int flags = FTW_PHYS;
    if (nftw(dir_path, nftw_callback, 20, flags) == -1) {
        perror("nftw failed");
        g_nftw_context = NULL; // Reset global context pointer
        return 0; // Failure
    }

    g_nftw_context = NULL; // Reset global context pointer
    return 1; // Success
}