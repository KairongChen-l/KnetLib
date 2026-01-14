#include "knetlib/Logger.h"
#include "knetlib/TcpServer.h"
#include "knetlib/EventLoop.h"
#include "knetlib/InetAddress.h"
#include <thread>
#include <chrono>

// 演示异步日志的使用
int main() {
    // 启用异步日志
    // basename: 日志文件基础名称
    // rollSize: 日志文件滚动大小（500MB）
    // flushInterval: 刷新间隔（3秒）
    setAsyncLogging("/tmp/knetlib_demo", 500 * 1024 * 1024, 3);
    
    setLogLevel(LOG_LEVEL::LOG_LEVEL_INFO);
    
    INFO("=== Async Logging Demo Started ===");
    
    // 模拟高并发日志写入
    const int numThreads = 5;
    const int messagesPerThread = 100;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i, messagesPerThread]() {
            for (int j = 0; j < messagesPerThread; ++j) {
                INFO("Thread %d: Message %d - This is a test log message", i, j);
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    INFO("=== All threads completed ===");
    
    // 等待异步日志写入完成
    std::this_thread::sleep_for(std::chrono::milliseconds(4000));
    
    INFO("=== Demo finished, check /tmp/knetlib_demo.log ===");
    
    // 禁用异步日志（在程序退出前）
    disableAsyncLogging();
    
    return 0;
}

