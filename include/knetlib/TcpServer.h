#pragma once

#include <atomic>
#include <memory>

#include "TcpServerSingle.h"
#include "EventLoopThreadPool.h"
#include "noncopyable.h"
#include "Callbacks.h"
#include "InetAddress.h"

class EventLoop;

class TcpServer : noncopyable {

public:
    TcpServer(EventLoop* loop, const InetAddress& local);
    ~TcpServer();

    // 设置工作线程数量（不包括主线程）
    // 必须在 start() 之前调用
    void setNumThread(size_t n);

    void start();

    void setThreadInitCallback(const ThreadInitCallback&);
    void setConnectionCallback(const ConnectionCallback&);
    void setMessageCallback(const MessageCallback&);
    void setWriteCompleteCallback(const WriteCompleteCallback&);

private:
    void startInLoop();
    // 新连接回调（在主线程中调用）
    void newConnection(int sockfd, const InetAddress& local, const InetAddress& peer);
    // 关闭连接回调（在连接所属的线程中调用）
    void closeConnection(const TcpConnectionPtr& conn);

    EventLoop* baseLoop_;
    std::unique_ptr<TcpServerSingle> acceptor_;  // 只在主线程中，负责 accept
    std::unique_ptr<EventLoopThreadPool> threadPool_;
    std::atomic_bool started_;
    InetAddress local_;
    ThreadInitCallback threadInitCallback_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    
    // 连接管理（每个连接属于一个 EventLoop）
    // 注意：连接的实际管理在各自的 EventLoop 线程中
};

