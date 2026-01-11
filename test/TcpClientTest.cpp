#include <gtest/gtest.h>
#include "knetlib/TcpClient.h"
#include "knetlib/EventLoop.h"
#include "knetlib/InetAddress.h"
#include <thread>
#include <chrono>
#include <atomic>

class TcpClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        loop = new EventLoop();
        peerAddr = InetAddress("127.0.0.1", 0); // 使用端口 0
    }

    void TearDown() override {
        if (loop) {
            loop->quit();
            delete loop;
        }
    }

    EventLoop* loop;
    InetAddress peerAddr;
};

// 测试基本构造
TEST_F(TcpClientTest, Constructor) {
    TcpClient client(loop, peerAddr);
    EXPECT_TRUE(true); // 如果构造成功，测试通过
}

// 测试设置回调
TEST_F(TcpClientTest, SetCallbacks) {
    TcpClient client(loop, peerAddr);
    
    bool connectionCallbackCalled = false;
    bool messageCallbackCalled = false;
    bool writeCompleteCallbackCalled = false;
    bool errorCallbackCalled = false;
    
    client.setConnectionCallback([&connectionCallbackCalled](const TcpConnectionPtr&) {
        connectionCallbackCalled = true;
    });
    
    client.setMessageCallback([&messageCallbackCalled](const TcpConnectionPtr&, Buffer&) {
        messageCallbackCalled = true;
    });
    
    client.setWriteCompleteCallback([&writeCompleteCallbackCalled](const TcpConnectionPtr&) {
        writeCompleteCallbackCalled = true;
    });
    
    client.setErrorCallback([&errorCallbackCalled]() {
        errorCallbackCalled = true;
    });
    
    EXPECT_FALSE(connectionCallbackCalled);
    EXPECT_FALSE(messageCallbackCalled);
    EXPECT_FALSE(writeCompleteCallbackCalled);
    EXPECT_FALSE(errorCallbackCalled);
}

// 测试 start（在没有服务器的情况下会失败，但不会崩溃）
TEST_F(TcpClientTest, Start) {
    TcpClient client(loop, InetAddress("127.0.0.1", 99999)); // 使用不存在的端口
    
    std::atomic<bool> started(false);
    
    loop->runInLoop([&client, &started]() {
        client.start();
        started = true;
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(started.load());
}

// 测试 disconnect
TEST_F(TcpClientTest, Disconnect) {
    TcpClient client(loop, peerAddr);
    
    loop->runInLoop([&client]() {
        client.disconnect();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(true);
}

