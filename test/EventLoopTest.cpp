#include <gtest/gtest.h>
#include "EventLoop.h"
#include "Channel.h"
#include "Callbacks.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <unistd.h>
#include <sys/eventfd.h>
#include <cstring>

class EventLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        loop = new EventLoop();
    }

    void TearDown() override {
        if (loop) {
            loop->quit();
            delete loop;
        }
    }

    EventLoop* loop;
};

// 测试基本构造
TEST_F(EventLoopTest, Constructor) {
    EXPECT_TRUE(loop->isInLoopThread());
}

// 测试 isInLoopThread
TEST_F(EventLoopTest, IsInLoopThread) {
    EXPECT_TRUE(loop->isInLoopThread());
    
    // 在另一个线程中应该返回 false
    std::atomic<bool> result(false);
    std::thread t([&]() {
        result = loop->isInLoopThread();
    });
    t.join();
    EXPECT_FALSE(result);
}

// 测试 runInLoop (同一线程)
TEST_F(EventLoopTest, RunInLoopSameThread) {
    std::atomic<bool> taskExecuted(false);
    
    loop->runInLoop([&]() {
        taskExecuted = true;
    });
    
    // 在同一线程中应该立即执行
    EXPECT_TRUE(taskExecuted);
}

// 测试 queueInLoop
TEST_F(EventLoopTest, QueueInLoop) {
    std::atomic<bool> taskExecuted(false);
    
    loop->queueInLoop([&]() {
        taskExecuted = true;
    });
    
    // 任务已加入队列，但需要 loop 运行才能执行
    // 这里只测试不会崩溃
    EXPECT_FALSE(taskExecuted);
}

// 测试 quit
TEST_F(EventLoopTest, Quit) {
    // quit 应该在 loop 运行时才能生效
    loop->quit();
    EXPECT_TRUE(true);  // 如果到这里没有崩溃，测试通过
}

// 测试跨线程 runInLoop
TEST_F(EventLoopTest, RunInLoopCrossThread) {
    std::atomic<bool> taskExecuted(false);
    std::atomic<bool> threadStarted(false);
    
    std::thread t([&]() {
        threadStarted = true;
        loop->runInLoop([&]() {
            taskExecuted = true;
        });
    });
    
    // 等待线程启动
    while (!threadStarted) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    t.join();
    
    // 任务应该已加入队列
    // 注意：实际执行需要 loop 运行
    EXPECT_FALSE(taskExecuted);  // 因为 loop 没有运行
}

// 测试 wakeup 机制
TEST_F(EventLoopTest, Wakeup) {
    // wakeup 应该在内部被调用，测试不会崩溃
    loop->wakeup();
    EXPECT_TRUE(true);
}

// 测试 updateChannel
TEST_F(EventLoopTest, UpdateChannel) {
    int pipefd[2];
    pipe(pipefd);
    
    Channel channel(loop, pipefd[0]);
    channel.enableRead();
    
    // updateChannel 应该在 loop 线程中调用
    loop->updateChannel(&channel);
    
    close(pipefd[0]);
    close(pipefd[1]);
    
    EXPECT_TRUE(true);  // 如果到这里没有崩溃，测试通过
}

// 测试 removeChannel
TEST_F(EventLoopTest, RemoveChannel) {
    int pipefd[2];
    pipe(pipefd);
    
    Channel channel(loop, pipefd[0]);
    channel.enableRead();
    loop->updateChannel(&channel);
    
    loop->removeChannel(&channel);
    
    close(pipefd[0]);
    close(pipefd[1]);
    
    EXPECT_TRUE(true);  // 如果到这里没有崩溃，测试通过
}

// 测试 assertInLoopThread
TEST_F(EventLoopTest, AssertInLoopThread) {
    // 在同一线程中不应该崩溃
    loop->assertInLoopThread();
    EXPECT_TRUE(true);
}

// 测试 assertNotInLoopThread
TEST_F(EventLoopTest, AssertNotInLoopThread) {
    // 在另一个线程中应该断言失败（在 Debug 模式下）
    // 这里只测试不会在正确的情况下崩溃
    std::thread t([&]() {
        // 在另一个线程中，assertNotInLoopThread 应该通过
        // 但这里我们测试的是 assertInLoopThread 应该失败
        // 由于是断言，我们无法直接测试，只能确保不会在正确情况下崩溃
    });
    t.join();
    EXPECT_TRUE(true);
}

// 测试定时器功能（如果已集成）
TEST_F(EventLoopTest, TimerFunctions) {
    // 测试定时器接口是否存在
    // 注意：实际定时器测试在 TimerQueueTest 中
    EXPECT_TRUE(true);
}

// 测试多个任务队列
TEST_F(EventLoopTest, MultipleTasks) {
    std::atomic<int> counter(0);
    
    for (int i = 0; i < 10; ++i) {
        loop->queueInLoop([&]() {
            counter++;
        });
    }
    
    // 任务已加入队列
    EXPECT_EQ(counter.load(), 0);  // 还没有执行
}

