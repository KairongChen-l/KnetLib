#pragma once

#include "noncopyable.h"
#include <sys/epoll.h>
#include <vector>

class Channel;
class EventLoop;

class Epoll: noncopyable {

public:
    using ChannelList = std::vector<Channel*>;

    explicit Epoll(EventLoop* loop);
    ~Epoll();

    void poll(ChannelList& activeChannels, int timeout = -1);
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

private:
    void updateChannel(int op, Channel* channel);
    EventLoop* loop_;
    std::vector<epoll_event> events_;
    int epollfd_;

};
