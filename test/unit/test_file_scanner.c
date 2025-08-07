/**
 * 文件扫描模块单元测试 / File Scanner Module Unit Tests
 * 
 * 本文件包含对 file_scanner.c 中文件扫描功能的单元测试
 * This file contains unit tests for file scanning functionality in file_scanner.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"
#include "file_scanner.h"

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
 * 创建测试目录结构 / Create test directory structure
 */
void setup_test_directory() {
    // 创建测试目录 / Create test directory
    system("mkdir -p /tmp/lra_test_dir");
    system("mkdir -p /tmp/lra_test_dir/subdir");
    
    // 创建测试文件 / Create test files
    system("touch /tmp/lra_test_dir/test1.mp3");
    system("touch /tmp/lra_test_dir/test2.flac");
    system("touch /tmp/lra_test_dir/test3.wav");
    system("touch /tmp/lra_test_dir/subdir/test4.m4a");
    system("touch /tmp/lra_test_dir/document.txt");  // 不支持的格式 / Unsupported format
    system("touch /tmp/lra_test_dir/image.jpg");     // 不支持的格式 / Unsupported format
}

/**
 * 清理测试目录 / Cleanup test directory
 */
void cleanup_test_directory() {
    system("rm -rf /tmp/lra_test_dir");
}

/**
 * 测试文件扫描功能 / Test file scanning functionality
 */
void test_file_scanning() {
    printf("\n=== 测试文件扫描功能 / Testing file scanning functionality ===\n");
    
    setup_test_directory();
    
    DynamicArrayFiles files;
    TEST_ASSERT(DynamicArrayFiles_init(&files, 10) == 1, "文件数组初始化");
    
    // 扫描测试目录 / Scan test directory
    int result = scan_files_with_nftw("/tmp/lra_test_dir", &files, "/tmp/lra_test_dir", "/tmp/lra_test_dir/results.txt");
    TEST_ASSERT(result == 1, "文件扫描执行成功");
    
    // 验证扫描结果 / Verify scan results
    TEST_ASSERT(files.count == 4, "找到正确数量的音频文件");
    
    // 验证文件路径 / Verify file paths
    int found_mp3 = 0, found_flac = 0, found_wav = 0, found_m4a = 0;
    for (size_t i = 0; i < files.count; i++) {
        if (strstr(files.items[i].relative_path, "test1.mp3")) found_mp3 = 1;
        if (strstr(files.items[i].relative_path, "test2.flac")) found_flac = 1;
        if (strstr(files.items[i].relative_path, "test3.wav")) found_wav = 1;
        if (strstr(files.items[i].relative_path, "test4.m4a")) found_m4a = 1;
    }
    
    TEST_ASSERT(found_mp3, "找到 MP3 文件");
    TEST_ASSERT(found_flac, "找到 FLAC 文件");
    TEST_ASSERT(found_wav, "找到 WAV 文件");
    TEST_ASSERT(found_m4a, "找到 M4A 文件");
    
    DynamicArrayFiles_free(&files);
    cleanup_test_directory();
}

/**
 * 测试空目录扫描 / Test empty directory scanning
 */
void test_empty_directory_scanning() {
    printf("\n=== 测试空目录扫描 / Testing empty directory scanning ===\n");
    
    system("mkdir -p /tmp/lra_empty_test");
    
    DynamicArrayFiles files;
    TEST_ASSERT(DynamicArrayFiles_init(&files, 10) == 1, "文件数组初始化");
    
    int result = scan_files_with_nftw("/tmp/lra_empty_test", &files, "/tmp/lra_empty_test", "/tmp/lra_empty_test/results.txt");
    TEST_ASSERT(result == 1, "空目录扫描执行成功");
    TEST_ASSERT(files.count == 0, "空目录中没有找到文件");
    
    DynamicArrayFiles_free(&files);
    system("rm -rf /tmp/lra_empty_test");
}

/**
 * 测试不存在目录的处理 / Test handling of non-existent directory
 */
void test_nonexistent_directory() {
    printf("\n=== 测试不存在目录的处理 / Testing non-existent directory handling ===\n");
    
    DynamicArrayFiles files;
    TEST_ASSERT(DynamicArrayFiles_init(&files, 10) == 1, "文件数组初始化");
    
    int result = scan_files_with_nftw("/tmp/nonexistent_directory", &files, "/tmp/nonexistent_directory", "/tmp/results.txt");
    TEST_ASSERT(result == 0, "不存在目录扫描应该失败");
    
    DynamicArrayFiles_free(&files);
}

/**
 * 主测试函数 / Main test function
 */
int main() {
    printf("开始运行文件扫描模块单元测试 / Starting file scanner module unit tests\n");
    printf("================================================================\n");
    
    test_file_scanning();
    test_empty_directory_scanning();
    test_nonexistent_directory();
    
    printf("\n================================================================\n");
    printf("所有测试通过！/ All tests passed!\n");
    
    return 0;
}
