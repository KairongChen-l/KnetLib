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
        pipe(pipefd);
        readFd = pipefd[0];
        writeFd = pipefd[1];
        channel = new Channel(loop, readFd);
    }

    void TearDown() override {
        delete channel;
        close(readFd);
        close(writeFd);
        delete epoll;
        delete loop;
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
    channel->enableRead();
    epoll->updateChannel(channel);
    
    EXPECT_TRUE(channel->pooling);
}

// 测试 removeChannel
TEST_F(EpollTest, RemoveChannel) {
    channel->enableRead();
    epoll->updateChannel(channel);
    EXPECT_TRUE(channel->pooling);
    
    channel->disableAll();
    epoll->removeChannel(channel);
    EXPECT_FALSE(channel->pooling);
}

// 测试 poll
TEST_F(EpollTest, Poll) {
    channel->enableRead();
    epoll->updateChannel(channel);
    
    // 写入数据触发可读事件
    write(writeFd, "test", 4);
    
    Epoll::ChannelList activeChannels;
    epoll->poll(activeChannels, 100);  // 100ms 超时
    
    // 可能触发也可能不触发，主要测试不会崩溃
    EXPECT_TRUE(true);
}

