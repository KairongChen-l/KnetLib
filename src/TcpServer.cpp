#include "knetlib/TcpServer.h"
#include "knetlib/EventLoop.h"
#include "knetlib/EventLoopThreadPool.h"
#include "knetlib/TcpConnection.h"
#include "knetlib/Logger.h"
#include <cassert>

TcpServer::TcpServer(EventLoop* loop, const InetAddress& local)
        : baseLoop_(loop),
          started_(false),
          local_(local)
{
    assert(baseLoop_ != nullptr);
    INFO("create TcpServer %s", local.toIpPort().c_str());
    
    // 创建线程池（默认只有主线程）
    threadPool_ = std::make_unique<EventLoopThreadPool>(baseLoop_);
}

TcpServer::~TcpServer() {
    // 停止 acceptor
    if (acceptor_ && baseLoop_) {
        if (baseLoop_->isInLoopThread()) {
            acceptor_->stop();
        } else {
            baseLoop_->runInLoop([this]() {
                if (acceptor_) {
                    acceptor_->stop();
                }
            });
        }
    }
    
    TRACE("~TcpServer");
}

void TcpServer::setNumThread(size_t n) {
    assert(!started_);
    if (n > 0) {
        threadPool_->setThreadNum(static_cast<int>(n));
    } else {
        ERROR("TcpServer::setNumThread n <= 0");
    }
}

void TcpServer::start() {
    if (started_.exchange(true)) return;

    baseLoop_->runInLoop([this](){startInLoop();});
}

void TcpServer::setThreadInitCallback(const ThreadInitCallback& callback) {
    threadInitCallback_ = callback;
}
void TcpServer::setConnectionCallback(const ConnectionCallback& callback) {
    connectionCallback_ = callback;
}
void TcpServer::setMessageCallback(const MessageCallback& callback) {
    messageCallback_ = callback;
}
void TcpServer::setWriteCompleteCallback(const WriteCompleteCallback& callback) {
    writeCompleteCallback_ = callback;
}

void TcpServer::startInLoop() {
    assert(baseLoop_->isInLoopThread());
    assert(!acceptor_);
    
    // 启动线程池
    threadPool_->start();
    
    // 调用线程初始化回调
    if (threadInitCallback_) {
        threadInitCallback_(0);  // 主线程
        // 为每个工作线程调用初始化回调
        auto loops = threadPool_->getAllLoops();
        for (size_t i = 1; i < loops.size(); ++i) {
            loops[i]->runInLoop([this, i]() {
                if (threadInitCallback_) {
                    threadInitCallback_(i);
                }
            });
        }
    }
    
    // 创建 Acceptor（只在主线程中）
    acceptor_ = std::make_unique<TcpServerSingle>(baseLoop_, local_);
    
    // 设置新连接回调（在主线程中 accept，然后分配给子线程）
    acceptor_->setNewConnectionCallback([this](int sockfd, const InetAddress& local, const InetAddress& peer) {
        newConnection(sockfd, local, peer);
    });
    
    // 启动监听
    acceptor_->start();
    
    INFO("TcpServer::start() %s with %zu eventLoop thread(s)", 
         local_.toIpPort().c_str(), 
         threadPool_->getAllLoops().size());
}

// 在主线程中调用，accept 新连接后分配给子线程
void TcpServer::newConnection(int sockfd, const InetAddress& local, const InetAddress& peer) {
    assert(baseLoop_->isInLoopThread());
    
    // 获取下一个 EventLoop（轮询分配）
    EventLoop* ioLoop = threadPool_->getNextLoop();
    
    // 在 ioLoop 的线程中创建连接
    ioLoop->runInLoop([this, sockfd, local, peer, ioLoop]() {
        // 创建连接
        auto conn = std::make_shared<TcpConnection>(ioLoop, sockfd, local, peer);
        
        // 设置回调
        conn->setMessageCallback(messageCallback_);
        conn->setWriteCompleteCallback(writeCompleteCallback_);
        conn->setCloseCallback([this](const TcpConnectionPtr& conn) {
            closeConnection(conn);
        });
        
        // 建立连接
        conn->connectEstablished();
        
        // 调用连接回调
        if (connectionCallback_) {
            connectionCallback_(conn);
        }
    });
}

// 在连接所属的线程中调用
void TcpServer::closeConnection(const TcpConnectionPtr& conn) {
    // 调用连接回调（通知连接关闭）
    if (connectionCallback_) {
        connectionCallback_(conn);
    }
    // 注意：连接的清理由 TcpConnection 自己负责
}

