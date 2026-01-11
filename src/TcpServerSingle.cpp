#include "knetlib/TcpServerSingle.h"
#include "knetlib/TcpConnection.h"
#include "knetlib/Buffer.h"
#include "knetlib/EventLoop.h"
#include "knetlib/Logger.h"
#include <thread>

TcpServerSingle::TcpServerSingle(EventLoop* loop, const InetAddress& local)
        : loop_(loop),
          acceptor_(loop, local)
{
    acceptor_.setNewConnectionCallback(std::bind(&TcpServerSingle::newConnection, this, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

TcpServerSingle::~TcpServerSingle() {
    // 关闭所有连接
    // 如果 EventLoop 还在运行，使用 forceClose（异步关闭）
    // 如果 EventLoop 已经退出，需要直接关闭 socket
    if (loop_->isInLoopThread()) {
        // 在 EventLoop 线程中，可以直接关闭
        for (auto& conn : connections_) {
            if (conn && !conn->disconnected()) {
                conn->forceClose();
            }
        }
        // 处理待处理的任务，确保关闭操作完成
        loop_->runInLoop([](){});
    } else {
        // 不在 EventLoop 线程中，尝试使用 queueInLoop
        // 但如果 EventLoop 已经退出，queueInLoop 可能无法执行
        // 这种情况下，我们只能直接关闭 socket
        for (auto& conn : connections_) {
            if (conn && !conn->disconnected()) {
                // 如果 EventLoop 已经退出，直接关闭 socket
                // 注意：这会导致连接状态不是 kDisconnected，但至少 socket 会被关闭
                conn->forceClose();
            }
        }
    }
    
    // 清空连接集合，shared_ptr 会自动管理生命周期
    connections_.clear();
}

void TcpServerSingle::setConnectionCallback(const ConnectionCallback& callback) {
    connectionCallback_ = callback;
}
void TcpServerSingle::setMessageCallback(const MessageCallback& callback) {
    messageCallback_ = callback;
}
void TcpServerSingle::setWriteCompleteCallback(const WriteCompleteCallback& callback) {
    writeCompleteCallback_ = callback;
}

void TcpServerSingle::start() {
    acceptor_.listen();
}

void TcpServerSingle::stop() {
    loop_->assertInLoopThread();
    // 关闭所有连接
    // 注意：我们在 EventLoop 线程中，forceClose 会检查 isInLoopThread()
    // 如果返回 true，会直接调用 forceCloseInLoop，而不是使用 queueInLoop
    // 这样可以确保连接被正确关闭，即使 EventLoop 已经退出
    for (auto& conn : connections_) {
        if (conn && !conn->disconnected()) {
            conn->forceClose();
        }
    }
    // 处理待处理的任务，确保关闭操作完成
    loop_->runInLoop([](){});
}

// 这里的逻辑将会传递给acceptor_.setNewConnectionCallback，当acceptfd_有可读事件触发，即有新连接请求到来时，就执行该逻辑
void TcpServerSingle::newConnection(int connfd, const InetAddress& local, const InetAddress& peer) {
    loop_->assertInLoopThread();
    auto conn = std::make_shared<TcpConnection>(loop_, connfd, local, peer);
    connections_.insert(conn);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServerSingle::closeConnection, this, std::placeholders::_1));
    
    // 将connfd的channel tie上TcpConnection对象
    conn->connectEstablished();

    if (connectionCallback_) {
        connectionCallback_(conn);
    }
}

void TcpServerSingle::closeConnection(const TcpConnectionPtr& conn) {
    loop_->assertInLoopThread();
    // 先调用回调通知连接关闭（此时连接可能还未完全断开）
    if (connectionCallback_) {
        connectionCallback_(conn);
    }
    // 然后从连接集合中删除
    size_t ret = connections_.erase(conn);
    if (ret != 1) {
        FATAL("TcpServerSingle::closeConnection connection set erase fatal, ret = %zu", ret);
    }
}

