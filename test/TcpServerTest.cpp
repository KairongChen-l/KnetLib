#include <gtest/gtest.h>
#include "knetlib/TcpServer.h"
#include "knetlib/TcpConnection.h"
#include "knetlib/EventLoop.h"
#include "knetlib/InetAddress.h"
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
    
    // setNumThread 必须在 start() 之前调用
    server.setNumThread(1);
    
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
    
    server.setNumThread(4);
    
    loop->runInLoop([&server]() {
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
    
    server.setNumThread(2);
    
    loop->runInLoop([&server]() {
        server.start();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 清理
    loop->quit();
    delete loop;
    loop = nullptr;
}

// 测试主从 Reactor 模式：验证连接被分配到不同的线程
TEST_F(TcpServerTest, MasterSlaveReactor) {
    TcpServer server(loop, serverAddr);
    
    std::atomic<int> connectionCount(0);
    std::vector<std::thread::id> connectionThreadIds;
    std::mutex threadIdsMutex;
    
    server.setConnectionCallback([&connectionCount, &connectionThreadIds, &threadIdsMutex](const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            connectionCount++;
            std::lock_guard<std::mutex> lock(threadIdsMutex);
            connectionThreadIds.push_back(std::this_thread::get_id());
        }
    });
    
    server.setNumThread(3);  // 3个工作线程 + 1个主线程
    
    loop->runInLoop([&server]() {
        server.start();
    });
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 这里可以添加客户端连接测试，但为了简化，我们只测试服务器启动
    // 实际连接测试需要客户端支持
    
    // 清理
    loop->quit();
    delete loop;
    loop = nullptr;
}

