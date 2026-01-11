#pragma once

#include "Callbacks.h"
#include "Connector.h"
#include "Timer.h"
#include "noncopyable.h"
#include <memory>

class EventLoop;
class TcpConnection;
class Timer;

class TcpClient : noncopyable {

public:
    TcpClient(EventLoop* loop, const InetAddress& peer);
    ~TcpClient();

    void setConnectionCallback(const ConnectionCallback&);
    void setMessageCallback(const MessageCallback&);
    void setWriteCompleteCallback(const WriteCompleteCallback&);
    void setErrorCallback(const ErrorCallback&);

    void start();
    void disconnect(); // 断开连接

private:
    void retry();
    void newConnection(int connfd, const InetAddress& local, const InetAddress& peer);
    void closeConnection(const TcpConnectionPtr& conn);

    using ConnectorPtr = std::unique_ptr<Connector>;

    EventLoop* loop_;
    bool connected_;
    const InetAddress peer_;
    Timer* retryTimer_;
    ConnectorPtr connector_;
    TcpConnectionPtr connection_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ErrorCallback errorCallback_;
};

