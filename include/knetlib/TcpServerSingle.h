#pragma once

#include <unordered_set>

#include "Callbacks.h"
#include "Acceptor.h"
#include "noncopyable.h"

class EventLoop;

class TcpServerSingle : noncopyable {

public:
    TcpServerSingle(EventLoop* loop, const InetAddress& local);
    ~TcpServerSingle();

    void setConnectionCallback(const ConnectionCallback& callback);
    void setMessageCallback(const MessageCallback &callback);
    void setWriteCompleteCallback(const WriteCompleteCallback &callback);
    // 设置新连接回调（用于主从 Reactor 模式）
    void setNewConnectionCallback(const NewConnectionCallback& callback);
    
    void start();
    void stop(); // 停止服务器，关闭所有连接

private:
    using ConnectionSet = std::unordered_set<TcpConnectionPtr>;

    void newConnection(int connfd, const InetAddress &local, const InetAddress &peer);

    void closeConnection(const TcpConnectionPtr &conn);

    EventLoop* loop_;
    Acceptor acceptor_;
    ConnectionSet connections_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
};

