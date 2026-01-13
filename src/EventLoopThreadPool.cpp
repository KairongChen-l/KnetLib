#include "knetlib/EventLoopThreadPool.h"
#include "knetlib/EventLoop.h"
#include "knetlib/Logger.h"
#include <cassert>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop)
        : baseLoop_(baseLoop),
          numThreads_(0),
          next_(0),
          started_(false)
{
    assert(baseLoop_ != nullptr);
}

EventLoopThreadPool::~EventLoopThreadPool() {
    // 注意：不要在这里调用 stop()，因为 EventLoop 可能已经退出
    // 让 EventLoopThread 的析构函数自动处理
}

void EventLoopThreadPool::start() {
    assert(!started_);
    assert(baseLoop_->isInLoopThread());
    
    started_ = true;
    
    // 创建并启动工作线程
    for (int i = 0; i < numThreads_; ++i) {
        auto thread = std::make_unique<EventLoopThread>();
        EventLoop* loop = thread->startLoop();
        threads_.push_back(std::move(thread));
        loops_.push_back(loop);
    }
    
    INFO("EventLoopThreadPool::start() with %d threads", numThreads_);
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    assert(started_);
    assert(baseLoop_->isInLoopThread());
    
    EventLoop* loop = baseLoop_;
    
    // 如果有工作线程，使用轮询算法分配
    if (!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        if (static_cast<size_t>(next_) >= loops_.size()) {
            next_ = 0;
        }
    }
    
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    assert(started_);
    std::vector<EventLoop*> allLoops;
    allLoops.push_back(baseLoop_);
    for (auto loop : loops_) {
        allLoops.push_back(loop);
    }
    return allLoops;
}

