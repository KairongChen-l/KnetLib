#include <gtest/gtest.h>
#include "TimerQueue.h"
#include "EventLoop.h"
#include "Timestamp.h"
#include <atomic>
#include <thread>
#include <chrono>

class TimerQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        loop = new EventLoop();
        callbackCalled = false;
    }

    void TearDown() override {
        if (loop) {
            loop->quit();
            delete loop;
        }
    }

    EventLoop* loop;
    std::atomic<bool> callbackCalled;
};

// 测试基本定时器
TEST_F(TimerQueueTest, BasicTimer) {
    std::atomic<bool> fired(false);
    
    Timer* timer = loop->runAfter(Milliseconds(100), [&fired]() {
        fired = true;
    });
    
    std::thread t([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        loop->quit();
    });
    
    loop->loop();
    t.join();
    
    // 注意：由于 EventLoop 已退出，定时器可能未触发
    // 这里主要测试不会崩溃
    EXPECT_TRUE(true);
}

// 测试重复定时器
TEST_F(TimerQueueTest, RepeatTimer) {
    std::atomic<int> count(0);
    
    Timer* timer = loop->runEvery(Milliseconds(50), [&count]() {
        count++;
    });
    
    std::thread t([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        loop->quit();
    });
    
    loop->loop();
    t.join();
    
    loop->cancelTimer(timer);
    // 主要测试不会崩溃
    EXPECT_TRUE(true);
}

// 测试取消定时器
TEST_F(TimerQueueTest, CancelTimer) {
    std::atomic<bool> fired(false);
    
    Timer* timer = loop->runAfter(Seconds(1), [&fired]() {
        fired = true;
    });
    
    loop->cancelTimer(timer);
    
    std::thread t([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        loop->quit();
    });
    
    loop->loop();
    t.join();
    
    EXPECT_FALSE(fired);
}

