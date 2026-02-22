# 测试构建配置 / Test Build Configuration
cmake_minimum_required(VERSION 3.10)
project(LRA-Calculator-Tests C)

# 启用测试 / Enable testing
enable_testing()

# 查找测试框架 / Find test framework
# 这里使用简单的断言测试，不依赖外部框架 / Using simple assertion tests without external frameworks

# 包含主项目的源文件 / Include main project source files
set(MAIN_SOURCES
    ../src/utils.c
    ../src/file_scanner.c
    ../src/ffmpeg_processor.c
    ../src/result_manager.c
    ../src/cache_manager.c
)

# 包含目录 / Include directories
include_directories(../src)

# 编译器标志 / Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g -O0 -DDEBUG")
endif()

# 单元测试 / Unit Tests
add_executable(test_utils
    unit/test_utils.c
    ../src/utils.c
)

add_executable(test_file_scanner
    unit/test_file_scanner.c
    ../src/file_scanner.c
    ../src/utils.c
)

add_executable(test_result_manager
    unit/test_result_manager.c
    ../src/result_manager.c
    ../src/utils.c
)

add_executable(test_cache_manager
    unit/test_cache_manager.c
    ../src/cache_manager.c
    ../src/utils.c
)

# 链接 pthread 库 / Link pthread library
find_package(Threads REQUIRED)
target_link_libraries(test_utils PRIVATE Threads::Threads)
target_link_libraries(test_file_scanner PRIVATE Threads::Threads)
target_link_libraries(test_result_manager PRIVATE Threads::Threads)
target_link_libraries(test_cache_manager PRIVATE Threads::Threads)

# 添加测试 / Add tests
add_test(NAME utils_tests COMMAND test_utils)
add_test(NAME file_scanner_tests COMMAND test_file_scanner)
add_test(NAME result_manager_tests COMMAND test_result_manager)
add_test(NAME cache_manager_tests COMMAND test_cache_manager)

# 集成测试 / Integration Tests
add_executable(integration_test
    integration/test_integration.c
    ${MAIN_SOURCES}
)

target_link_libraries(integration_test PRIVATE Threads::Threads)

# macOS 平台特定配置 / macOS platform specific configuration
if(APPLE)
    # 可能需要链接 Dispatch 框架 / May need to link Dispatch framework
    # target_link_libraries(integration_test PRIVATE "-framework Dispatch")
endif()

# Linux 平台可能需要 libdispatch / Linux platforms may need libdispatch
if(UNIX AND NOT APPLE)
    find_library(DISPATCH_LIBRARY dispatch)
    if(DISPATCH_LIBRARY)
        target_link_libraries(integration_test PRIVATE ${DISPATCH_LIBRARY})
    endif()
endif()

add_test(NAME integration_tests COMMAND integration_test)

# 设置测试数据目录 / Set test data directory
set_tests_properties(utils_tests file_scanner_tests integration_tests
    PROPERTIES
    ENVIRONMENT "TEST_DATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}/data"
)

# 自定义测试目标 / Custom test targets
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS test_utils test_file_scanner test_result_manager test_cache_manager integration_test
    COMMENT "Running all tests"
)

add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose -R "utils_tests|file_scanner_tests|result_manager_tests|cache_manager_tests"
    DEPENDS test_utils test_file_scanner test_result_manager test_cache_manager
    COMMENT "Running unit tests only"
)

add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose -R "integration_tests"
    DEPENDS integration_test
    COMMENT "Running integration tests only"
)
