#include <gtest/gtest.h>
#include "TcpServerSingle.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <thread>
#include <chrono>
#include <atomic>

class TcpServerSingleTest : public ::testing::Test {
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
TEST_F(TcpServerSingleTest, Constructor) {
    TcpServerSingle server(loop, serverAddr);
    EXPECT_TRUE(true); // 如果构造成功，测试通过
}

// 测试设置回调
TEST_F(TcpServerSingleTest, SetCallbacks) {
    TcpServerSingle server(loop, serverAddr);
    
    bool connectionCallbackCalled = false;
    bool messageCallbackCalled = false;
    bool writeCompleteCallbackCalled = false;
    
    server.setConnectionCallback([&connectionCallbackCalled](const TcpConnectionPtr&) {
        connectionCallbackCalled = true;
    });
    
    server.setMessageCallback([&messageCallbackCalled](const TcpConnectionPtr&, Buffer&) {
        messageCallbackCalled = true;
    });
    
    server.setWriteCompleteCallback([&writeCompleteCallbackCalled](const TcpConnectionPtr&) {
        writeCompleteCallbackCalled = true;
    });
    
    EXPECT_FALSE(connectionCallbackCalled);
    EXPECT_FALSE(messageCallbackCalled);
    EXPECT_FALSE(writeCompleteCallbackCalled);
}

// 测试 start
TEST_F(TcpServerSingleTest, Start) {
    TcpServerSingle server(loop, serverAddr);
    
    loop->runInLoop([&server]() {
        server.start();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(true);
}

// 测试 stop
TEST_F(TcpServerSingleTest, Stop) {
    TcpServerSingle server(loop, serverAddr);
    
    loop->runInLoop([&server]() {
        server.start();
        server.stop();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(true);
}

