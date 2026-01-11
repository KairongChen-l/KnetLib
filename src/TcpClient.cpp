#include "knetlib/TcpClient.h"
#include "knetlib/Logger.h"
#include "knetlib/EventLoop.h"
#include "knetlib/TcpConnection.h"
#include "knetlib/Timestamp.h"
#include <chrono>

using namespace std::chrono;

TcpClient::TcpClient(EventLoop* loop, const InetAddress& peer)
        : loop_(loop),
          connected_(false),
          peer_(peer),
          retryTimer_(nullptr),
          connector_(new Connector(loop, peer))
{
    connector_->setNewConnectionCallback(std::bind(
            &TcpClient::newConnection, 
            this, 
            std::placeholders::_1, 
            std::placeholders::_2,
            std::placeholders::_3
    ));
}

TcpClient::~TcpClient() {
    if (connection_ && !connection_->disconnected()) {
        connection_->forceClose();
    }
    if (retryTimer_ != nullptr) {
        loop_->cancelTimer(retryTimer_);
    }
}

void TcpClient::setConnectionCallback(const ConnectionCallback& callback) {
    connectionCallback_ = callback;
}
void TcpClient::setMessageCallback(const MessageCallback& callback) {
    messageCallback_ = callback;
}
void TcpClient::setWriteCompleteCallback(const WriteCompleteCallback& callback) {
    writeCompleteCallback_ = callback;
}
void TcpClient::setErrorCallback(const ErrorCallback& callback) {
    errorCallback_ = callback;
    if (connector_) {
        connector_->setErrorCallback(callback);
    }
}

void TcpClient::start() {
    loop_->assertInLoopThread();
    connector_->start();
    retryTimer_ = loop_->runEvery(Seconds(3), [this](){retry();});
}

void TcpClient::retry() {
    loop_->assertInLoopThread();
    if (connected_) return;

    WARN("TcpClient::retry() reconnect %s...", peer_.toIpPort().c_str());
    // 先重置旧的 connector，确保资源正确释放
    connector_.reset();
    connector_ = std::make_unique<Connector>(loop_, peer_);
    connector_->setNewConnectionCallback(std::bind(
            &TcpClient::newConnection, 
            this, 
            std::placeholders::_1, 
            std::placeholders::_2, 
            std::placeholders::_3
    ));
    // 设置错误回调（如果有的话）
    if (errorCallback_) {
        connector_->setErrorCallback(errorCallback_);
    }
    connector_->start();
}

void TcpClient::newConnection(int connfd, const InetAddress& local, const InetAddress& peer) {
    loop_->assertInLoopThread();
    loop_->cancelTimer(retryTimer_);
    retryTimer_ = nullptr;
    connected_ = true;
    auto conn = std::make_shared<TcpConnection>(loop_, connfd, local, peer);
    connection_ = conn;
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpClient::closeConnection, this, std::placeholders::_1));

    conn->connectEstablished();
    if (connectionCallback_) {
        connectionCallback_(conn);
    }
}

void TcpClient::disconnect() {
    if (connection_ && !connection_->disconnected()) {
        if (loop_->isInLoopThread()) {
            connection_->shutdown();
        } else {
            loop_->runInLoop([this]() {
                if (connection_ && !connection_->disconnected()) {
                    connection_->shutdown();
                }
            });
        }
    }
}

void TcpClient::closeConnection(const TcpConnectionPtr& conn) {
    loop_->assertInLoopThread();
    connected_ = false;
    // 注意：connection_ 可能已经被重置，所以先保存回调
    ConnectionCallback cb = connectionCallback_;
    connection_.reset();
    if (cb) {
        cb(conn);
    }
}

