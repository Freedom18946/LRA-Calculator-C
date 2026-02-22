/**
 * 集成测试 / Integration Tests
 * 
 * 本文件包含对整个 LRA-Calculator-C 系统的集成测试
 * This file contains integration tests for the entire LRA-Calculator-C system
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"
#include "file_scanner.h"
#include "ffmpeg_processor.h"
#include "result_manager.h"

// 简单的测试框架宏 / Simple test framework macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "测试失败 / Test failed: %s\n", message); \
            fprintf(stderr, "文件 / File: %s, 行 / Line: %d\n", __FILE__, __LINE__); \
            exit(1); \
        } else { \
            printf("✓ 测试通过 / Test passed: %s\n", message); \
        } \
    } while(0)

/**
 * 创建测试音频文件 / Create test audio files
 * 注意：这里创建的是空文件，实际测试需要真实的音频文件
 * Note: These are empty files, real testing would need actual audio files
 */
void setup_test_audio_files() {
    system("mkdir -p /tmp/lra_integration_test");
    
    // 创建模拟音频文件 / Create mock audio files
    system("touch /tmp/lra_integration_test/test1.mp3");
    system("touch /tmp/lra_integration_test/test2.flac");
    system("touch /tmp/lra_integration_test/test3.wav");
}

/**
 * 清理测试文件 / Cleanup test files
 */
void cleanup_test_files() {
    system("rm -rf /tmp/lra_integration_test");
}

/**
 * 测试完整的工作流程 / Test complete workflow
 */
void test_complete_workflow() {
    printf("\n=== 测试完整工作流程 / Testing complete workflow ===\n");
    
    setup_test_audio_files();
    
    // 1. 初始化数据结构 / Initialize data structures
    DynamicArrayFiles files;
    DynamicArrayResults results;
    
    TEST_ASSERT(DynamicArrayFiles_init(&files, 10) == 1, "文件数组初始化");
    TEST_ASSERT(DynamicArrayResults_init(&results, 10) == 1, "结果数组初始化");
    
    // 2. 扫描文件 / Scan files
    int scan_result = scan_files_with_nftw("/tmp/lra_integration_test", &files, 
                                          "/tmp/lra_integration_test", 
                                          "/tmp/lra_integration_test/results.txt",
                                          NULL);
    TEST_ASSERT(scan_result == 1, "文件扫描成功");
    TEST_ASSERT(files.count == 3, "找到预期数量的文件");
    
    // 3. 处理文件 (注意：由于是空文件，FFmpeg 处理会失败，这是预期的)
    // Process files (Note: Since these are empty files, FFmpeg processing will fail, which is expected)
    for (size_t i = 0; i < files.count; i++) {
        ProcessResult result;
        FFmpegExecutionConfig config = {.timeout_seconds = 10, .max_cpu_seconds = 10, .max_memory_mb = 512};
        calculate_lra_for_file(&files.items[i], &result, &config);
        
        // 空文件处理应该失败 / Empty file processing should fail
        TEST_ASSERT(result.success == 0, "空文件处理失败（预期行为）");
        TEST_ASSERT(strlen(result.error_message) > 0, "错误消息不为空");
        
        DynamicArrayResults_add(&results, &result);
    }
    
    // 4. 写入结果文件 / Write results file
    int write_result = write_results_to_file("/tmp/lra_integration_test/test_results.txt", 
                                            &results, LRA_HEADER_LINE);
    TEST_ASSERT(write_result == 1, "结果文件写入成功");
    
    // 5. 验证结果文件存在 / Verify result file exists
    struct stat st;
    TEST_ASSERT(stat("/tmp/lra_integration_test/test_results.txt", &st) == 0, "结果文件存在");
    
    // 清理 / Cleanup
    DynamicArrayFiles_free(&files);
    DynamicArrayResults_free(&results);
    cleanup_test_files();
}

/**
 * 测试错误处理 / Test error handling
 */
void test_error_handling() {
    printf("\n=== 测试错误处理 / Testing error handling ===\n");
    
    // 测试无效参数 / Test invalid parameters
    ProcessResult result;
    FFmpegExecutionConfig config = {.timeout_seconds = 10, .max_cpu_seconds = 10, .max_memory_mb = 512};
    calculate_lra_for_file(NULL, &result, &config);
    TEST_ASSERT(result.success == 0, "空指针参数处理");
    TEST_ASSERT(strlen(result.error_message) > 0, "错误消息设置正确");
    
    // 测试不存在的文件 / Test non-existent file
    FileToProcess fake_file = {"/tmp/nonexistent_file.mp3", "nonexistent_file.mp3", 0, 0};
    calculate_lra_for_file(&fake_file, &result, &config);
    TEST_ASSERT(result.success == 0, "不存在文件处理失败");
    TEST_ASSERT(strlen(result.error_message) > 0, "错误消息不为空");
}

/**
 * 测试内存管理 / Test memory management
 */
void test_memory_management() {
    printf("\n=== 测试内存管理 / Testing memory management ===\n");
    
    // 测试动态数组的扩容 / Test dynamic array expansion
    DynamicArrayFiles files;
    TEST_ASSERT(DynamicArrayFiles_init(&files, 2) == 1, "小容量初始化");
    
    // 添加超过初始容量的文件 / Add more files than initial capacity
    for (int i = 0; i < 5; i++) {
        FileToProcess file;
        snprintf(file.full_path, sizeof(file.full_path), "/tmp/test%d.mp3", i);
        snprintf(file.relative_path, sizeof(file.relative_path), "test%d.mp3", i);
        
        TEST_ASSERT(DynamicArrayFiles_add(&files, &file) == 1, "文件添加成功");
    }
    
    TEST_ASSERT(files.count == 5, "所有文件都已添加");
    TEST_ASSERT(files.capacity >= 5, "容量已正确扩展");
    
    // 验证数据完整性 / Verify data integrity
    for (size_t i = 0; i < files.count; i++) {
        char expected[64];
        snprintf(expected, sizeof(expected), "test%zu.mp3", i);
        TEST_ASSERT(strcmp(files.items[i].relative_path, expected) == 0, "数据完整性保持");
    }
    
    DynamicArrayFiles_free(&files);
    TEST_ASSERT(files.items == NULL, "内存正确释放");
    TEST_ASSERT(files.count == 0, "计数器重置");
    TEST_ASSERT(files.capacity == 0, "容量重置");
}

/**
 * 主测试函数 / Main test function
 */
int main() {
    printf("开始运行集成测试 / Starting integration tests\n");
    printf("================================================\n");
    
    test_complete_workflow();
    test_error_handling();
    test_memory_management();
    
    printf("\n================================================\n");
    printf("所有集成测试通过！/ All integration tests passed!\n");
    
    return 0;
}
