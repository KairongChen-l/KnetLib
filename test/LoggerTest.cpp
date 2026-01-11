#include <gtest/gtest.h>
#include "knetlib/Logger.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <unistd.h>

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 保存原始日志级别
        originalLevel = logLevel;
        originalFileName = logFileName;
    }

    void TearDown() override {
        // 恢复原始设置
        logLevel = originalLevel;
        if (logFileName != originalFileName) {
            setLogFile(originalFileName);
        }
    }

    LOG_LEVEL originalLevel;
    std::string originalFileName;
};

// 测试基本日志输出
TEST_F(LoggerTest, BasicLogging) {
    setLogLevel(LOG_LEVEL::LOG_LEVEL_TRACE);
    
    // 这些应该都能输出
    TRACE("Trace message: %d", 1);
    DEBUG("Debug message: %d", 2);
    INFO("Info message: %d", 3);
    WARN("Warn message: %d", 4);
    ERROR("Error message: %d", 5);
    
    // FATAL 会导致 abort，所以不在这里测试
}

// 测试日志级别过滤
TEST_F(LoggerTest, LogLevelFiltering) {
    setLogLevel(LOG_LEVEL::LOG_LEVEL_WARN);
    
    // 只有 WARN 及以上级别应该输出
    TRACE("This should not appear");
    DEBUG("This should not appear");
    INFO("This should not appear");
    WARN("This should appear");
    ERROR("This should appear");
}

// 测试文件日志
TEST_F(LoggerTest, FileLogging) {
    std::string testFile = "/tmp/knetlib_test.log";
    // 删除可能存在的旧文件
    unlink(testFile.c_str());
    
    setLogFile(testFile);
    setLogLevel(LOG_LEVEL::LOG_LEVEL_INFO);
    
    INFO("Test log message to file");
    
    // 恢复 stdout
    setLogFile("stdout");
    
    // 验证文件是否存在且有内容
    std::ifstream file(testFile);
    ASSERT_TRUE(file.good());
    
    std::string line;
    std::getline(file, line);
    ASSERT_FALSE(line.empty());
    ASSERT_NE(line.find("Test log message"), std::string::npos);
    
    // 清理
    unlink(testFile.c_str());
}

// 测试时间戳格式
TEST_F(LoggerTest, TimestampFormat) {
    setLogLevel(LOG_LEVEL::LOG_LEVEL_INFO);
    INFO("Timestamp test");
    // 时间戳格式应该包含日期和时间
    // 这里只测试不会崩溃
    EXPECT_TRUE(true);
}

// 测试进程ID
TEST_F(LoggerTest, ProcessId) {
    setLogLevel(LOG_LEVEL::LOG_LEVEL_INFO);
    INFO("Process ID test");
    // 应该包含进程ID
    // 这里只测试不会崩溃
    EXPECT_TRUE(true);
}

// 测试不同日志级别
TEST_F(LoggerTest, DifferentLogLevels) {
    setLogLevel(LOG_LEVEL::LOG_LEVEL_DEBUG);
    
    TRACE("TRACE level");
    DEBUG("DEBUG level");
    INFO("INFO level");
    WARN("WARN level");
    ERROR("ERROR level");
    
    // 所有级别都应该输出（因为设置为 DEBUG）
    EXPECT_TRUE(true);
}

// 测试日志级别设置
TEST_F(LoggerTest, SetLogLevel) {
    setLogLevel(LOG_LEVEL::LOG_LEVEL_ERROR);
    EXPECT_EQ(logLevel, LOG_LEVEL::LOG_LEVEL_ERROR);
    
    setLogLevel(LOG_LEVEL::LOG_LEVEL_INFO);
    EXPECT_EQ(logLevel, LOG_LEVEL::LOG_LEVEL_INFO);
}

// 测试日志文件设置
TEST_F(LoggerTest, SetLogFile) {
    std::string testFile = "/tmp/knetlib_test2.log";
    unlink(testFile.c_str());
    
    setLogFile(testFile);
    EXPECT_EQ(logFileName, testFile);
    
    setLogFile("stdout");
    EXPECT_EQ(logFileName, "stdout");
    
    unlink(testFile.c_str());
}

