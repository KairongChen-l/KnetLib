#pragma once

#include "noncopyable.h"
#include "EventLoop.h"
#include <thread>
#include <mutex>
#include <condition_variable>

class EventLoopThread : noncopyable {
public:
    EventLoopThread();
    ~EventLoopThread();

    // 启动线程并返回 EventLoop 指针
    // 线程安全，可以在任何线程调用
    EventLoop* startLoop();

    // 停止线程和 EventLoop
    void stop();

private:
    // 线程函数
    void threadFunc();

    EventLoop* loop_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool exiting_;
};

