#include <gtest/gtest.h>
#include "knetlib/Connector.h"
#include "knetlib/EventLoop.h"
#include "knetlib/InetAddress.h"
#include <thread>
#include <chrono>
#include <atomic>

class ConnectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        loop = new EventLoop();
        peerAddr = InetAddress("127.0.0.1", 0);
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
TEST_F(ConnectorTest, Constructor) {
    Connector connector(loop, peerAddr);
    EXPECT_TRUE(true); // 如果构造成功，测试通过
}

// 测试 setNewConnectionCallback
TEST_F(ConnectorTest, SetNewConnectionCallback) {
    Connector connector(loop, peerAddr);
    
    std::atomic<bool> callbackCalled(false);
    
    connector.setNewConnectionCallback([&callbackCalled](int, const InetAddress&, const InetAddress&) {
        callbackCalled = true;
    });
    
    EXPECT_FALSE(callbackCalled.load());
}

// 测试 setErrorCallback
TEST_F(ConnectorTest, SetErrorCallback) {
    Connector connector(loop, peerAddr);
    
    std::atomic<bool> callbackCalled(false);
    
    connector.setErrorCallback([&callbackCalled]() {
        callbackCalled = true;
    });
    
    EXPECT_FALSE(callbackCalled.load());
}

// 测试 start（在没有服务器的情况下会失败，但不会崩溃）
TEST_F(ConnectorTest, Start) {
    Connector connector(loop, InetAddress("127.0.0.1", 99999)); // 使用不存在的端口
    
    std::atomic<bool> started(false);
    
    loop->runInLoop([&connector, &started]() {
        connector.start();
        started = true;
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(started.load());
}

