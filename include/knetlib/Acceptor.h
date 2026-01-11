#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Channel.h"
#include "Callbacks.h"

class EventLoop;

class Acceptor: noncopyable {

public:
    Acceptor(EventLoop* loop, const InetAddress& local);
    ~Acceptor();

    bool listening() const;

    void listen();

    void setNewConnectionCallback(const NewConnectionCallback& callback);

private:
    void handleRead();
    
    bool listening_;
    EventLoop* loop_; // 指向的是主Reactor的EventLoop对象
    const int acceptfd_;
    Channel acceptChannel_;
    InetAddress local_;
    NewConnectionCallback newConnectionCallback_;
};
