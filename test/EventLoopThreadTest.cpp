#include <gtest/gtest.h>
#include "knetlib/EventLoopThread.h"
#include "knetlib/EventLoop.h"
#include <thread>
#include <chrono>
#include <atomic>

class EventLoopThreadTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// 测试基本构造和启动
TEST_F(EventLoopThreadTest, StartLoop) {
    EventLoopThread thread;
    
    EventLoop* loop = thread.startLoop();
    
    EXPECT_NE(loop, nullptr);
    
    // 从另一个线程检查 isInLoopThread
    std::atomic<bool> isInLoopThread(false);
    loop->runInLoop([&isInLoopThread, loop]() {
        isInLoopThread = loop->isInLoopThread();
    });
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(isInLoopThread.load());
    
    // 停止线程
    thread.stop();
}

// 测试跨线程调用
TEST_F(EventLoopThreadTest, CrossThreadCall) {
    EventLoopThread thread;
    
    EventLoop* loop = thread.startLoop();
    
    std::atomic<bool> taskExecuted(false);
    
    // 从另一个线程调用
    std::thread t([loop, &taskExecuted]() {
        loop->runInLoop([&taskExecuted]() {
            taskExecuted = true;
        });
    });
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(taskExecuted.load());
    
    t.join();
    thread.stop();
}

// 测试停止后重新创建
TEST_F(EventLoopThreadTest, StopAndRestart) {
    EventLoopThread thread1;
    
    EventLoop* loop1 = thread1.startLoop();
    EXPECT_NE(loop1, nullptr);
    
    // 停止线程
    thread1.stop();
    
    // 重新创建线程
    EventLoopThread thread2;
    EventLoop* loop2 = thread2.startLoop();
    EXPECT_NE(loop2, nullptr);
    // 注意：由于 EventLoop 是栈上对象，地址可能相同，但对象不同
    // 我们主要验证可以正常启动和停止
    
    thread2.stop();
}

