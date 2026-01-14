#pragma once

#include "noncopyable.h"
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class AsyncLogging : noncopyable {
public:
    AsyncLogging(const std::string& basename,
                 off_t rollSize = 500 * 1024 * 1024,  // 500MB
                 int flushInterval = 3);  // 3秒刷新一次
    
    ~AsyncLogging();

    // 追加日志到缓冲区
    void append(const char* logline, int len);

    // 启动异步日志
    void start();

    // 停止异步日志
    void stop();

private:
    // 日志滚动
    void rollFile();

    // 后台线程函数
    void threadFunc();

    // 固定大小的缓冲区类型
    using Buffer = std::vector<char>;
    using BufferPtr = std::shared_ptr<Buffer>;
    using BufferVector = std::vector<BufferPtr>;

    const std::string basename_;      // 日志文件基础名称
    const off_t rollSize_;            // 日志文件滚动大小
    const int flushInterval_;         // 刷新间隔（秒）

    std::atomic<bool> running_;       // 是否运行中

    std::thread thread_;              // 后台线程
    std::mutex mutex_;                // 保护缓冲区的互斥锁
    std::condition_variable cond_;   // 条件变量

    BufferPtr currentBuffer_;        // 当前缓冲区（前端）
    BufferPtr nextBuffer_;            // 下一个缓冲区（前端备用）
    BufferVector buffers_;            // 待写入的缓冲区队列（后端）

    static const int kBufferSize = 64 * 1024;  // 64KB 缓冲区
};

