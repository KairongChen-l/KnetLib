#include <gtest/gtest.h>
#include "knetlib/Timer.h"
#include "knetlib/Timestamp.h"
#include "knetlib/Callbacks.h"
#include <atomic>
#include <thread>
#include <chrono>

class TimerTest : public ::testing::Test {
protected:
    void SetUp() override {
        callbackCalled = false;
    }

    void TearDown() override {
    }

    std::atomic<bool> callbackCalled;
};

// 测试 Timer 构造
TEST_F(TimerTest, Constructor) {
    TimerCallback cb = [this]() { callbackCalled = true; };
    Timestamp when = time_utils::now() + Seconds(1);
    Timer timer(cb, when, Milliseconds::zero());
    
    EXPECT_FALSE(timer.repeat());
    EXPECT_FALSE(timer.canceled());
    EXPECT_EQ(timer.when(), when);
}

// 测试重复定时器
TEST_F(TimerTest, RepeatTimer) {
    TimerCallback cb = [this]() { callbackCalled = true; };
    Timestamp when = time_utils::now() + Seconds(1);
    Timer timer(cb, when, Seconds(1));
    
    EXPECT_TRUE(timer.repeat());
}

// 测试过期检查
TEST_F(TimerTest, Expired) {
    TimerCallback cb = []() {};
    Timestamp past = time_utils::now() - Seconds(1);
    Timestamp future = time_utils::now() + Seconds(1);
    
    Timer timer1(cb, past, Milliseconds::zero());
    Timer timer2(cb, future, Milliseconds::zero());
    
    EXPECT_TRUE(timer1.expired(time_utils::now()));
    EXPECT_FALSE(timer2.expired(time_utils::now()));
}

// 测试 run
TEST_F(TimerTest, Run) {
    TimerCallback cb = [this]() { callbackCalled = true; };
    Timer timer(cb, time_utils::now(), Milliseconds::zero());
    
    timer.run();
    EXPECT_TRUE(callbackCalled);
}

// 测试 restart
TEST_F(TimerTest, Restart) {
    TimerCallback cb = []() {};
    Timestamp when = time_utils::now();
    Timer timer(cb, when, Seconds(1));
    
    Timestamp oldWhen = timer.when();
    timer.restart();
    Timestamp newWhen = timer.when();
    
    EXPECT_GT(newWhen, oldWhen);
    EXPECT_GE((newWhen - oldWhen).count(), Seconds(1).count() - Milliseconds(100).count());
}

// 测试 cancel
TEST_F(TimerTest, Cancel) {
    TimerCallback cb = []() {};
    Timer timer(cb, time_utils::now(), Milliseconds::zero());
    
    EXPECT_FALSE(timer.canceled());
    timer.cancel();
    EXPECT_TRUE(timer.canceled());
}

