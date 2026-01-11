#include <gtest/gtest.h>
#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <thread>
#include <chrono>
#include <atomic>

class AcceptorTest : public ::testing::Test {
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
TEST_F(AcceptorTest, Constructor) {
    Acceptor acceptor(loop, serverAddr);
    EXPECT_FALSE(acceptor.listening());
}

// 测试 listen
TEST_F(AcceptorTest, Listen) {
    Acceptor acceptor(loop, serverAddr);
    
    loop->runInLoop([&acceptor]() {
        acceptor.listen();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(acceptor.listening());
}

// 测试 setNewConnectionCallback
TEST_F(AcceptorTest, SetNewConnectionCallback) {
    Acceptor acceptor(loop, serverAddr);
    
    std::atomic<bool> callbackCalled(false);
    
    acceptor.setNewConnectionCallback([&callbackCalled](int, const InetAddress&, const InetAddress&) {
        callbackCalled = true;
    });
    
    EXPECT_FALSE(callbackCalled.load());
}

// 测试监听状态
TEST_F(AcceptorTest, ListeningState) {
    Acceptor acceptor(loop, serverAddr);
    
    EXPECT_FALSE(acceptor.listening());
    
    loop->runInLoop([&acceptor]() {
        acceptor.listen();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(acceptor.listening());
}

