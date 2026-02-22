cmake_minimum_required(VERSION 3.10)
project(LRA-Calculator-C C)

# 设置 C 标准 / Set C Standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译器警告标志 / Compiler Warning Flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Wformat=2 -Wstrict-prototypes -Wmissing-prototypes")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshadow -Wpointer-arith -Wcast-qual -Wwrite-strings")

    # Debug 模式下的额外检查 / Additional checks in Debug mode
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG -fsanitize=address -fno-omit-frame-pointer")

    # Release 模式优化 / Release mode optimizations
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# 源文件列表 / Source files list
set(SOURCES
    src/main.c
    src/utils.c
    src/file_scanner.c
    src/ffmpeg_processor.c
    src/result_manager.c
    src/cache_manager.c
)

# 创建可执行文件 / Create executable
add_executable(LRA-Calculator-C ${SOURCES})

# 包含目录 / Include directories
target_include_directories(LRA-Calculator-C PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 查找并链接 pthread 库 / Find and link pthread library
# pthread 用于互斥锁保护共享结果数据 / pthread used for mutex protecting shared result data
find_package(Threads REQUIRED)
target_link_libraries(LRA-Calculator-C PRIVATE Threads::Threads)

# macOS 平台特定配置 / macOS platform specific configuration
if(APPLE)
    # Grand Central Dispatch (GCD) 在 macOS 上通常不需要特殊链接
    # GCD on macOS typically doesn't require special linking
    # 如果出现链接错误，取消注释下面的行 / If linker errors occur, uncomment the line below
    # target_link_libraries(LRA-Calculator-C PRIVATE "-framework Dispatch")
endif()

# Linux 平台可能需要 libdispatch / Linux platforms may need libdispatch
if(UNIX AND NOT APPLE)
    find_library(DISPATCH_LIBRARY dispatch)
    if(DISPATCH_LIBRARY)
        target_link_libraries(LRA-Calculator-C PRIVATE ${DISPATCH_LIBRARY})
        message(STATUS "Found libdispatch: ${DISPATCH_LIBRARY}")
    else()
        message(WARNING "libdispatch not found. GCD functionality may not work on this platform.")
    endif()
endif()
