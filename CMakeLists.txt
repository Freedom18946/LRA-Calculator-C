cmake_minimum_required(VERSION 3.10)
project(LRA-Calculator-C C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# POSIX nftw is part of libc.
# GCD (libdispatch) is typically available on macOS without special linking for C.
# If linker errors related to dispatch occur, you might need:
# target_link_libraries(LRA-Calculator-C PRIVATE "-framework Dispatch")
# or find_library(DISPATCH_LIBRARY dispatch) target_link_libraries(LRA-Calculator-C PRIVATE ${DISPATCH_LIBRARY})

add_executable(LRA-Calculator-C
        src/main.c
        src/utils.c
        src/file_scanner.c
        src/ffmpeg_processor.c
        src/result_manager.c
        src/result_manager.c
)

target_include_directories(LRA-Calculator-C PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# pthreads might still be needed if any utility or future feature uses it,
# but GCD largely replaces direct pthread use for task parallelism.
# For the mutex protecting results, pthreads is still used.
find_package(Threads REQUIRED)
target_link_libraries(LRA-Calculator-C PRIVATE Threads::Threads)
