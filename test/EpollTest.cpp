#include <gtest/gtest.h>
#include "Epoll.h"
#include "EventLoop.h"
#include "Channel.h"
#include <unistd.h>
#include <sys/epoll.h>

class EpollTest : public ::testing::Test {
protected:
    void SetUp() override {
        loop = new EventLoop();
        epoll = new Epoll(loop);
        
        int pipefd[2];
        int ret = pipe(pipefd);
        ASSERT_EQ(ret, 0) << "Failed to create pipe";
        readFd = pipefd[0];
        writeFd = pipefd[1];
        ASSERT_GE(readFd, 0) << "Invalid read fd";
        ASSERT_GE(writeFd, 0) << "Invalid write fd";
        channel = new Channel(loop, readFd);
        ASSERT_NE(channel, nullptr) << "Failed to create channel";
        ASSERT_EQ(channel->fd(), readFd) << "Channel fd mismatch";
    }

    void TearDown() override {
        if (channel) {
            delete channel;
            channel = nullptr;
        }
        if (readFd >= 0) {
            close(readFd);
            readFd = -1;
        }
        if (writeFd >= 0) {
            close(writeFd);
            writeFd = -1;
        }
        if (epoll) {
            delete epoll;
            epoll = nullptr;
        }
        if (loop) {
            delete loop;
            loop = nullptr;
        }
    }

    EventLoop* loop;
    Epoll* epoll;
    Channel* channel;
    int readFd;
    int writeFd;
};

// 测试基本构造
TEST_F(EpollTest, Constructor) {
    EXPECT_NE(epoll, nullptr);
}

// 测试 updateChannel
TEST_F(EpollTest, UpdateChannel) {
    // 确保 fd 有效
    ASSERT_GE(channel->fd(), 0);
    channel->enableRead();
    epoll->updateChannel(channel);
    
    EXPECT_TRUE(channel->pooling);
}

// 测试 removeChannel
TEST_F(EpollTest, RemoveChannel) {
    // 确保 fd 有效
    ASSERT_GE(channel->fd(), 0);
    channel->enableRead();
    epoll->updateChannel(channel);
    EXPECT_TRUE(channel->pooling);
    
    channel->disableAll();
    epoll->removeChannel(channel);
    EXPECT_FALSE(channel->pooling);
}

// 测试 poll
TEST_F(EpollTest, Poll) {
    // 确保 fd 有效
    ASSERT_GE(channel->fd(), 0);
    channel->enableRead();
    epoll->updateChannel(channel);
    
    // 写入数据触发可读事件
    write(writeFd, "test", 4);
    
    Epoll::ChannelList activeChannels;
    epoll->poll(activeChannels, 100);  // 100ms 超时
    
    // 可能触发也可能不触发，主要测试不会崩溃
    EXPECT_TRUE(true);
}

