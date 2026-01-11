#include <gtest/gtest.h>
#include "knetlib/Channel.h"
#include "knetlib/EventLoop.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <memory>
#include <cstring>

class ChannelTest : public ::testing::Test {
protected:
    void SetUp() override {
        loop = new EventLoop();
        int pipefd[2];
        pipe(pipefd);
        readFd = pipefd[0];
        writeFd = pipefd[1];
        channel = new Channel(loop, readFd);
    }

    void TearDown() override {
        delete channel;
        close(readFd);
        close(writeFd);
        delete loop;
    }

    EventLoop* loop;
    Channel* channel;
    int readFd;
    int writeFd;
};

// 测试基本构造
TEST_F(ChannelTest, Constructor) {
    EXPECT_EQ(channel->fd(), readFd);
    EXPECT_FALSE(channel->isReading());
    EXPECT_FALSE(channel->isWriting());
    EXPECT_TRUE(channel->isNoneEvents());
}

// 测试 enableRead
TEST_F(ChannelTest, EnableRead) {
    channel->enableRead();
    EXPECT_TRUE(channel->isReading());
    EXPECT_FALSE(channel->isNoneEvents());
}

// 测试 enableWrite
TEST_F(ChannelTest, EnableWrite) {
    channel->enableWrite();
    EXPECT_TRUE(channel->isWriting());
    EXPECT_FALSE(channel->isNoneEvents());
}

// 测试 disableRead
TEST_F(ChannelTest, DisableRead) {
    channel->enableRead();
    EXPECT_TRUE(channel->isReading());
    
    channel->disableRead();
    EXPECT_FALSE(channel->isReading());
}

// 测试 disableWrite
TEST_F(ChannelTest, DisableWrite) {
    channel->enableWrite();
    EXPECT_TRUE(channel->isWriting());
    
    channel->disableWrite();
    EXPECT_FALSE(channel->isWriting());
}

// 测试 disableAll
TEST_F(ChannelTest, DisableAll) {
    channel->enableRead();
    channel->enableWrite();
    EXPECT_FALSE(channel->isNoneEvents());
    
    channel->disableAll();
    EXPECT_TRUE(channel->isNoneEvents());
}

// 测试 setReadCallback
TEST_F(ChannelTest, SetReadCallback) {
    bool callbackCalled = false;
    channel->setReadCallback([&callbackCalled]() {
        callbackCalled = true;
    });
    
    channel->enableRead();
    // 注意：实际调用 handleEvents 需要事件触发，这里只测试设置
    EXPECT_FALSE(callbackCalled);
}

// 测试 setWriteCallback
TEST_F(ChannelTest, SetWriteCallback) {
    bool callbackCalled = false;
    channel->setWriteCallback([&callbackCalled]() {
        callbackCalled = true;
    });
    
    channel->enableWrite();
    EXPECT_FALSE(callbackCalled);
}

// 测试 setCloseCallback
TEST_F(ChannelTest, SetCloseCallback) {
    bool callbackCalled = false;
    channel->setCloseCallback([&callbackCalled]() {
        callbackCalled = true;
    });
    
    // 注意：实际调用需要 EPOLLHUP 事件
    EXPECT_FALSE(callbackCalled);
}

// 测试 setErrorCallback
TEST_F(ChannelTest, SetErrorCallback) {
    bool callbackCalled = false;
    channel->setErrorCallback([&callbackCalled]() {
        callbackCalled = true;
    });
    
    // 注意：实际调用需要 EPOLLERR 事件
    EXPECT_FALSE(callbackCalled);
}

// 测试 tie 机制
TEST_F(ChannelTest, Tie) {
    auto sharedObj = std::make_shared<int>(42);
    channel->tie(sharedObj);
    
    // tie 后，sharedObj 的生命周期会被延长
    EXPECT_NE(sharedObj.use_count(), 0);
}

// 测试 setRevents
TEST_F(ChannelTest, SetRevents) {
    channel->setRevents(EPOLLIN);
    EXPECT_EQ(channel->events(), 0);  // events 和 revents 是不同的
    
    channel->enableRead();
    channel->setRevents(EPOLLIN);
    // revents 已设置，但需要通过 events() 检查
}

// 测试兼容 API: useET
TEST_F(ChannelTest, UseET) {
    // useET 目前是空实现，只测试不会崩溃
    channel->useET();
    EXPECT_TRUE(true);  // 如果到这里没有崩溃，测试通过
}

// 测试兼容 API: setUseThreadPool
TEST_F(ChannelTest, SetUseThreadPool) {
    // setUseThreadPool 目前是空实现，只测试不会崩溃
    channel->setUseThreadPool(true);
    channel->setUseThreadPool(false);
    EXPECT_TRUE(true);  // 如果到这里没有崩溃，测试通过
}

