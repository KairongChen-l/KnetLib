#include <gtest/gtest.h>
#include "knetlib/EventLoopThreadPool.h"
#include "knetlib/EventLoop.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <set>

class EventLoopThreadPoolTest : public ::testing::Test {
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
TEST_F(EventLoopThreadPoolTest, Constructor) {
    EventLoopThreadPool pool(loop);
    EXPECT_FALSE(pool.started());
}

// 测试设置线程数
TEST_F(EventLoopThreadPoolTest, SetThreadNum) {
    EventLoopThreadPool pool(loop);
    
    pool.setThreadNum(4);
    EXPECT_FALSE(pool.started());
}

// 测试启动和获取 EventLoop
TEST_F(EventLoopThreadPoolTest, StartAndGetNextLoop) {
    EventLoopThreadPool pool(loop);
    EventLoopThreadPool pool2(loop);
    
    pool2.setThreadNum(3);
    
    loop->runInLoop([&pool2]() {
        pool2.start();
    });
    
    // 等待线程启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    loop->runInLoop([&pool2]() {
        EXPECT_TRUE(pool2.started());
        
        // 测试轮询分配
        EventLoop* loop1 = pool2.getNextLoop();
        EventLoop* loop2 = pool2.getNextLoop();
        EventLoop* loop3 = pool2.getNextLoop();
        EventLoop* loop4 = pool2.getNextLoop();
        
        EXPECT_NE(loop1, nullptr);
        EXPECT_NE(loop2, nullptr);
        EXPECT_NE(loop3, nullptr);
        EXPECT_NE(loop4, nullptr);
        
        // loop4 应该和 loop1 相同（轮询）
        EXPECT_EQ(loop1, loop4);
    });
    
    loop->quit();
}

// 测试获取所有 EventLoop
TEST_F(EventLoopThreadPoolTest, GetAllLoops) {
    EventLoopThreadPool pool(loop);
    
    pool.setThreadNum(2);
    
    loop->runInLoop([&pool]() {
        pool.start();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    loop->runInLoop([&pool, this]() {
        auto loops = pool.getAllLoops();
        
        // 应该有 1 个主线程 + 2 个工作线程 = 3 个
        EXPECT_EQ(loops.size(), 3);
        EXPECT_EQ(loops[0], loop);  // 第一个应该是主线程的 loop
        
        // 验证所有 loop 都不为空
        for (auto l : loops) {
            EXPECT_NE(l, nullptr);
        }
    });
    
    loop->quit();
}

// 测试轮询分配算法
TEST_F(EventLoopThreadPoolTest, RoundRobin) {
    EventLoopThreadPool pool(loop);
    
    pool.setThreadNum(3);
    
    loop->runInLoop([&pool]() {
        pool.start();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    loop->runInLoop([&pool]() {
        std::vector<EventLoop*> loops;
        
        // 获取 6 次，应该轮询分配
        for (int i = 0; i < 6; ++i) {
            loops.push_back(pool.getNextLoop());
        }
        
        // 验证轮询：第 0 个和第 3 个应该相同
        EXPECT_EQ(loops[0], loops[3]);
        EXPECT_EQ(loops[1], loops[4]);
        EXPECT_EQ(loops[2], loops[5]);
        
        // 验证所有 loop 都不为空
        for (auto l : loops) {
            EXPECT_NE(l, nullptr);
        }
    });
    
    loop->quit();
}

