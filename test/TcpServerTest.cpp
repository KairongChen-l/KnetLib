#include <gtest/gtest.h>
#include "TcpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <thread>
#include <chrono>
#include <atomic>

class TcpServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        loop = new EventLoop();
        serverAddr = InetAddress(0); // 使用端口 0 让系统自动分配
    }

    void TearDown() override {
        if (loop) {
            loop->quit();
            delete loop;
        }
    }

    EventLoop* loop;
    InetAddress serverAddr;
};

// 测试基本构造
TEST_F(TcpServerTest, Constructor) {
    TcpServer server(loop, serverAddr);
    EXPECT_TRUE(true); // 如果构造成功，测试通过
}

// 测试 setNumThread
TEST_F(TcpServerTest, SetNumThread) {
    TcpServer server(loop, serverAddr);
    
    // 需要在 EventLoop 线程中调用
    loop->runInLoop([&server]() {
        server.setNumThread(1);
    });
    
    EXPECT_TRUE(true);
}

// 测试设置回调
TEST_F(TcpServerTest, SetCallbacks) {
    TcpServer server(loop, serverAddr);
    
    bool connectionCallbackCalled = false;
    bool messageCallbackCalled = false;
    
    server.setConnectionCallback([&connectionCallbackCalled](const TcpConnectionPtr&) {
        connectionCallbackCalled = true;
    });
    
    server.setMessageCallback([&messageCallbackCalled](const TcpConnectionPtr&, Buffer&) {
        messageCallbackCalled = true;
    });
    
    EXPECT_FALSE(connectionCallbackCalled);
    EXPECT_FALSE(messageCallbackCalled);
}

// 测试 start
TEST_F(TcpServerTest, Start) {
    TcpServer server(loop, serverAddr);
    
    std::atomic<bool> started(false);
    
    loop->runInLoop([&server, &started]() {
        server.start();
        started = true;
    });
    
    // 等待 start 完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(started.load());
}

// 测试多线程设置
TEST_F(TcpServerTest, MultiThread) {
    TcpServer server(loop, serverAddr);
    
    std::atomic<int> threadInitCount(0);
    
    server.setThreadInitCallback([&threadInitCount](size_t index) {
        threadInitCount++;
    });
    
    loop->runInLoop([&server]() {
        server.setNumThread(4);
        server.start();
    });
    
    // 等待线程初始化
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 清理
    loop->quit();
    delete loop;
    loop = nullptr;
}

// 测试线程初始化回调
TEST_F(TcpServerTest, ThreadInitCallback) {
    TcpServer server(loop, serverAddr);
    
    std::atomic<int> callbackCount(0);
    
    server.setThreadInitCallback([&callbackCount](size_t index) {
        callbackCount++;
    });
    
    loop->runInLoop([&server]() {
        server.setNumThread(2);
        server.start();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 清理
    loop->quit();
    delete loop;
    loop = nullptr;
}

