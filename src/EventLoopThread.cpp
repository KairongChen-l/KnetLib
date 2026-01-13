#include "knetlib/EventLoopThread.h"
#include "knetlib/Logger.h"
#include <cassert>

EventLoopThread::EventLoopThread()
        : loop_(nullptr),
          exiting_(false)
{
}

EventLoopThread::~EventLoopThread() {
    stop();
}

EventLoop* EventLoopThread::startLoop() {
    assert(loop_ == nullptr);
    
    // 启动线程
    thread_ = std::thread(&EventLoopThread::threadFunc, this);
    
    // 等待线程创建 EventLoop 完成
    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr) {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    
    return loop;
}

void EventLoopThread::stop() {
    if (exiting_) {
        return;
    }
    
    exiting_ = true;
    
    if (loop_ != nullptr) {
        // 退出 EventLoop
        loop_->quit();
    }
    
    // 等待线程退出
    if (thread_.joinable()) {
        thread_.join();
    }
}

void EventLoopThread::threadFunc() {
    EventLoop loop;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    
    // 运行 EventLoop
    loop.loop();
    
    // EventLoop 退出后，清理
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = nullptr;
    }
    
    TRACE("EventLoopThread::threadFunc exit");
}

