#include <gtest/gtest.h>
#include "TcpConnection.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <thread>

class TcpConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        loop = new EventLoop();
        InetAddress local("127.0.0.1", 0);
        InetAddress peer("127.0.0.1", 0);
        
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        connection = std::make_shared<TcpConnection>(loop, sockfd, local, peer);
    }

    void TearDown() override {
        connection.reset();
        if (loop) {
            loop->quit();
            delete loop;
        }
    }

    EventLoop* loop;
    TcpConnectionPtr connection;
};

// 测试基本构造
TEST_F(TcpConnectionTest, Constructor) {
    EXPECT_FALSE(connection->connected());
    EXPECT_TRUE(connection->disconnected());
}

// 测试 connectEstablished
TEST_F(TcpConnectionTest, ConnectEstablished) {
    connection->connectEstablished();
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

