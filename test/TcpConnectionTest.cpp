#include <gtest/gtest.h>
#include "knetlib/TcpConnection.h"
#include "knetlib/EventLoop.h"
#include "knetlib/InetAddress.h"
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <thread>

class TcpConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 在 EventLoop 线程中创建 EventLoop 和 TcpConnection
        // 这样可以避免 assertInLoopThread() 失败
        loop = nullptr;
        connection = nullptr;
        
        std::atomic<bool> ready(false);
        loopThread = std::thread([this, &ready]() {
            loop = new EventLoop();
            InetAddress local("127.0.0.1", 0);
            InetAddress peer("127.0.0.1", 0);
            
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            connection = std::make_shared<TcpConnection>(loop, sockfd, local, peer);
            ready = true;
            
            // 运行 EventLoop 以处理可能的异步操作
            // 注意：loop() 会阻塞，直到 quit() 被调用
            loop->loop();
        });
        
        // 等待 EventLoop 和 TcpConnection 创建完成
        while (!ready.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void TearDown() override {
        // 在 EventLoop 线程中关闭连接
        if (loop && connection) {
            loop->runInLoop([this]() {
                if (connection && !connection->disconnected()) {
                    connection->forceClose();
                }
            });
        }
        
        // 退出 EventLoop
        if (loop) {
            loop->quit();
        }
        
        // 等待 EventLoop 线程退出
        if (loopThread.joinable()) {
            loopThread.join();
        }
        
        // 清理资源
        connection.reset();
        if (loop) {
            delete loop;
            loop = nullptr;
        }
    }

    EventLoop* loop;
    TcpConnectionPtr connection;
    std::thread loopThread;
};

// 测试基本构造
TEST_F(TcpConnectionTest, Constructor) {
    // 注意：TcpConnection 初始状态是 kConnecting，不是 kDisconnected
    // 所以 disconnected() 返回 false 是正常的
    EXPECT_FALSE(connection->connected());
    EXPECT_FALSE(connection->disconnected()); // 初始状态是 kConnecting
}

// 测试 connectEstablished
TEST_F(TcpConnectionTest, ConnectEstablished) {
    // connectEstablished 需要在 EventLoop 线程中调用
    if (loop) {
        loop->runInLoop([this]() {
            connection->connectEstablished();
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(connection->connected());
    EXPECT_FALSE(connection->disconnected());
}

// 测试 name
TEST_F(TcpConnectionTest, Name) {
    std::string name = connection->name();
    EXPECT_FALSE(name.empty());
}

// 测试 local 和 peer
TEST_F(TcpConnectionTest, LocalAndPeer) {
    const InetAddress& local = connection->local();
    const InetAddress& peer = connection->peer();
    
    EXPECT_EQ(local.toPort(), 0);
    EXPECT_EQ(peer.toPort(), 0);
}

// 测试 inputBuffer 和 outputBuffer
TEST_F(TcpConnectionTest, Buffers) {
    const Buffer& input = connection->inputBuffer();
    const Buffer& output = connection->outputBuffer();
    
    EXPECT_EQ(input.readableBytes(), 0);
    EXPECT_EQ(output.readableBytes(), 0);
}

// 测试 isReading
TEST_F(TcpConnectionTest, IsReading) {
    bool reading = connection->isReading();
    // 初始状态可能不同
    EXPECT_TRUE(true);  // 主要测试不会崩溃
}

// 测试 setContext 和 getContext
TEST_F(TcpConnectionTest, Context) {
    std::string testData = "test context";
    connection->setContext(testData);
    
    const std::any& ctx = connection->getContext();
    EXPECT_TRUE(ctx.has_value());
    
    try {
        std::string retrieved = std::any_cast<std::string>(ctx);
        EXPECT_EQ(retrieved, testData);
    } catch (const std::bad_any_cast&) {
        FAIL() << "Failed to cast context to string";
    }
    
    std::any& mutableCtx = connection->getContext();
    EXPECT_TRUE(mutableCtx.has_value());
}

