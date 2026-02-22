/**
 * 工具函数单元测试 / Utility Functions Unit Tests
 * 
 * 本文件包含对 utils.c 中各种工具函数的单元测试
 * This file contains unit tests for various utility functions in utils.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "utils.h"

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

#define TEST_ASSERT_STR_EQ(str1, str2, message) \
    do { \
        if (strcmp(str1, str2) != 0) { \
            fprintf(stderr, "测试失败 / Test failed: %s\n", message); \
            fprintf(stderr, "期望 / Expected: '%s', 实际 / Actual: '%s'\n", str2, str1); \
            fprintf(stderr, "文件 / File: %s, 行 / Line: %d\n", __FILE__, __LINE__); \
            exit(1); \
        } else { \
            printf("✓ 测试通过 / Test passed: %s\n", message); \
        } \
    } while(0)

/**
 * 测试文件扩展名提取函数 / Test file extension extraction function
 */
void test_get_filename_ext() {
    printf("\n=== 测试文件扩展名提取 / Testing file extension extraction ===\n");
    
    TEST_ASSERT_STR_EQ(get_filename_ext("test.mp3"), "mp3", "提取 .mp3 扩展名");
    TEST_ASSERT_STR_EQ(get_filename_ext("music.flac"), "flac", "提取 .flac 扩展名");
    TEST_ASSERT_STR_EQ(get_filename_ext("path/to/file.wav"), "wav", "从路径中提取扩展名");
    TEST_ASSERT_STR_EQ(get_filename_ext("noextension"), "", "无扩展名文件");
    TEST_ASSERT_STR_EQ(get_filename_ext(".hidden"), "", "隐藏文件无扩展名");
    TEST_ASSERT_STR_EQ(get_filename_ext("file."), "", "空扩展名");
    TEST_ASSERT_STR_EQ(get_filename_ext("file.tar.gz"), "gz", "多重扩展名");
}

/**
 * 测试字符串转小写函数 / Test string to lowercase function
 */
void test_string_to_lower() {
    printf("\n=== 测试字符串转小写 / Testing string to lowercase ===\n");
    
    char test1[] = "HELLO";
    string_to_lower(test1);
    TEST_ASSERT_STR_EQ(test1, "hello", "全大写转小写");
    
    char test2[] = "MiXeD CaSe";
    string_to_lower(test2);
    TEST_ASSERT_STR_EQ(test2, "mixed case", "混合大小写转小写");
    
    char test3[] = "already lowercase";
    string_to_lower(test3);
    TEST_ASSERT_STR_EQ(test3, "already lowercase", "已经是小写");
    
    char test4[] = "123ABC!@#";
    string_to_lower(test4);
    TEST_ASSERT_STR_EQ(test4, "123abc!@#", "数字和符号混合");
    
    // 测试空指针安全性 / Test null pointer safety
    string_to_lower(NULL);  // 应该不会崩溃 / Should not crash
    printf("✓ 测试通过 / Test passed: 空指针安全处理\n");
}

/**
 * 测试支持的文件格式检查 / Test supported file format checking
 */
void test_is_supported_extension() {
    printf("\n=== 测试支持的文件格式检查 / Testing supported file format checking ===\n");
    
    TEST_ASSERT(is_supported_extension("test.mp3") == 1, "MP3 格式支持");
    TEST_ASSERT(is_supported_extension("music.flac") == 1, "FLAC 格式支持");
    TEST_ASSERT(is_supported_extension("audio.wav") == 1, "WAV 格式支持");
    TEST_ASSERT(is_supported_extension("song.m4a") == 1, "M4A 格式支持");
    TEST_ASSERT(is_supported_extension("track.ogg") == 1, "OGG 格式支持");
    
    TEST_ASSERT(is_supported_extension("document.txt") == 0, "TXT 格式不支持");
    TEST_ASSERT(is_supported_extension("image.jpg") == 0, "JPG 格式不支持");
    TEST_ASSERT(is_supported_extension("video.mp4") == 0, "MP4 格式不支持");
    TEST_ASSERT(is_supported_extension("noextension") == 0, "无扩展名不支持");
    
    // 测试大小写不敏感 / Test case insensitive
    TEST_ASSERT(is_supported_extension("MUSIC.MP3") == 1, "大写扩展名支持");
    TEST_ASSERT(is_supported_extension("Song.FlAc") == 1, "混合大小写扩展名支持");
    
    // 测试空指针和空字符串 / Test null pointer and empty string
    TEST_ASSERT(is_supported_extension(NULL) == 0, "空指针处理");
    TEST_ASSERT(is_supported_extension("") == 0, "空字符串处理");
}

/**
 * 测试安全字符串复制函数 / Test safe string copy function
 */
void test_safe_strncpy() {
    printf("\n=== 测试安全字符串复制 / Testing safe string copy ===\n");
    
    char dest[10];
    
    // 正常复制 / Normal copy
    safe_strncpy(dest, "hello", sizeof(dest));
    TEST_ASSERT_STR_EQ(dest, "hello", "正常字符串复制");
    
    // 截断复制 / Truncated copy
    safe_strncpy(dest, "this is a very long string", sizeof(dest));
    TEST_ASSERT(strlen(dest) == 9, "字符串被正确截断");
    TEST_ASSERT(dest[9] == '\0', "确保空终止符");
    
    // 零长度缓冲区 / Zero length buffer
    char zero_dest[1];
    safe_strncpy(zero_dest, "test", 0);
    // 应该不会崩溃 / Should not crash
    printf("✓ 测试通过 / Test passed: 零长度缓冲区处理\n");
}

/**
 * 测试动态数组功能 / Test dynamic array functionality
 */
void test_dynamic_arrays() {
    printf("\n=== 测试动态数组功能 / Testing dynamic array functionality ===\n");
    
    // 测试文件数组 / Test file array
    DynamicArrayFiles files;
    TEST_ASSERT(DynamicArrayFiles_init(&files, 2) == 1, "文件数组初始化");
    TEST_ASSERT(files.count == 0, "初始计数为零");
    TEST_ASSERT(files.capacity == 2, "初始容量正确");
    
    // 添加文件 / Add files
    FileToProcess file1 = {"/path/to/file1.mp3", "file1.mp3", 0, 0};
    FileToProcess file2 = {"/path/to/file2.flac", "file2.flac", 0, 0};
    FileToProcess file3 = {"/path/to/file3.wav", "file3.wav", 0, 0};
    
    TEST_ASSERT(DynamicArrayFiles_add(&files, &file1) == 1, "添加第一个文件");
    TEST_ASSERT(files.count == 1, "计数更新正确");
    
    TEST_ASSERT(DynamicArrayFiles_add(&files, &file2) == 1, "添加第二个文件");
    TEST_ASSERT(files.count == 2, "计数更新正确");
    
    // 触发扩容 / Trigger expansion
    TEST_ASSERT(DynamicArrayFiles_add(&files, &file3) == 1, "添加第三个文件触发扩容");
    TEST_ASSERT(files.count == 3, "扩容后计数正确");
    TEST_ASSERT(files.capacity >= 3, "扩容后容量足够");
    
    // 验证数据完整性 / Verify data integrity
    TEST_ASSERT_STR_EQ(files.items[0].relative_path, "file1.mp3", "第一个文件数据正确");
    TEST_ASSERT_STR_EQ(files.items[1].relative_path, "file2.flac", "第二个文件数据正确");
    TEST_ASSERT_STR_EQ(files.items[2].relative_path, "file3.wav", "第三个文件数据正确");
    
    DynamicArrayFiles_free(&files);
    TEST_ASSERT(files.items == NULL, "数组释放后指针为空");
    TEST_ASSERT(files.count == 0, "数组释放后计数为零");
}

/**
 * 测试相对路径计算 / Test relative path calculation
 */
void test_get_relative_path() {
    printf("\n=== 测试相对路径计算 / Testing relative path calculation ===\n");
    
    char output[MAX_PATH_LEN];
    
    // 正常情况 / Normal case
    get_relative_path("/home/user/music", "/home/user/music/album/song.mp3", output, sizeof(output));
    TEST_ASSERT_STR_EQ(output, "album/song.mp3", "正常相对路径计算");
    
    // 基础路径末尾有斜杠 / Base path with trailing slash
    get_relative_path("/home/user/music/", "/home/user/music/album/song.mp3", output, sizeof(output));
    TEST_ASSERT_STR_EQ(output, "album/song.mp3", "基础路径有斜杠的情况");
    
    // 完全不匹配的路径 / Completely unmatched paths
    get_relative_path("/home/user/music", "/var/log/system.log", output, sizeof(output));
    TEST_ASSERT_STR_EQ(output, "/var/log/system.log", "不匹配路径返回原路径");
    
    // 相同路径 / Same path
    get_relative_path("/home/user/music", "/home/user/music", output, sizeof(output));
    TEST_ASSERT_STR_EQ(output, "", "相同路径返回空字符串");
}

/**
 * 主测试函数 / Main test function
 */
int main() {
    printf("开始运行工具函数单元测试 / Starting utility functions unit tests\n");
    printf("========================================================\n");
    
    test_get_filename_ext();
    test_string_to_lower();
    test_is_supported_extension();
    test_safe_strncpy();
    test_dynamic_arrays();
    test_get_relative_path();
    
    printf("\n========================================================\n");
    printf("所有测试通过！/ All tests passed!\n");
    
    return 0;
}
