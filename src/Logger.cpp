#include "knetlib/Logger.h"
#include "knetlib/AsyncLogging.h"
#include <cassert>

// 异步日志相关全局变量
std::unique_ptr<AsyncLogging> asyncLogging;
bool useAsyncLogging = false;

// 追加到异步日志
void appendToAsyncLog(const char* logline, int len) {
    if (asyncLogging) {
        asyncLogging->append(logline, len);
    }
}

// 启用异步日志的实现
void setAsyncLogging(const std::string& basename, off_t rollSize, int flushInterval) {
    // 如果已经启用，先停止
    if (asyncLogging) {
        asyncLogging->stop();
        asyncLogging.reset();
    }
    
    // 创建新的异步日志实例
    asyncLogging = std::make_unique<AsyncLogging>(basename, rollSize, flushInterval);
    asyncLogging->start();
    useAsyncLogging = true;
}

// 禁用异步日志
void disableAsyncLogging() {
    if (asyncLogging) {
        asyncLogging->stop();
        asyncLogging.reset();
    }
    useAsyncLogging = false;
}

