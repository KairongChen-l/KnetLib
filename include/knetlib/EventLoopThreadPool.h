#pragma once

#include "noncopyable.h"
#include "EventLoopThread.h"
#include <vector>
#include <memory>

class EventLoop;

class EventLoopThreadPool : noncopyable {
public:
    EventLoopThreadPool(EventLoop* baseLoop);
    ~EventLoopThreadPool();

    // 设置线程数量（不包括 baseLoop）
    // 必须在 start() 之前调用
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    
    // 启动线程池
    void start();

    // 获取下一个 EventLoop（用于连接分配）
    // 使用轮询算法
    EventLoop* getNextLoop();

    // 获取所有 EventLoop（包括 baseLoop）
    std::vector<EventLoop*> getAllLoops();

    // 是否已启动
    bool started() const { return started_; }

private:
    EventLoop* baseLoop_;  // 主线程的 EventLoop
    int numThreads_;      // 线程数量
    int next_;            // 下一个要使用的线程索引（用于轮询）
    bool started_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;  // 所有 EventLoop 的指针
};

