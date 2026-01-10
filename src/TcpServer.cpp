#include "TcpServer.h"
#include "EventLoop.h"
#include "Logger.h"
#include <cassert>

TcpServer::TcpServer(EventLoop* loop, const InetAddress& local)
        : baseLoop_(loop),
          numThreads_(1),
          started_(false),
          local_(local)
{
    assert(baseLoop_ != nullptr);
    INFO("create TcpServer %s", local.toIpPort().c_str());
}

TcpServer::~TcpServer() {
    for (auto& loop : eventLoops_) {
        if (loop != nullptr) {
            loop->quit();
        }
    }
    for (auto& thread : threads_) {
        if (thread && thread->joinable()) {
            thread->join();
        }
    }
    TRACE("~TcpServer");
}

void TcpServer::setNumThread(size_t n) {
    baseLoop_->assertInLoopThread();
    if (n > 0) {
        numThreads_ = n;
        eventLoops_.resize(n);
    }
    else {
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
    INFO("TcpServer::start() %s with %zu eventLoop thread(s)", local_.toIpPort().c_str(), numThreads_);
    
    baseServer_ = std::make_unique<TcpServerSingle>(baseLoop_, local_);
    /**
     * 如果想改主从Reactor结构，从这里入手，如果只有一个Reactor，则baseServer_回调即为外部传进来的回调，否则，baseServer_执行自己的回调（TcpServer中添加回调，
     * 来实现连接分配算法），由子Reactor执行外部传进来的回调。Connection对象绑定loop的步骤也需要修改位于TcpServerSingle.cpp
    **/
    baseServer_->setConnectionCallback(connectionCallback_);
    baseServer_->setMessageCallback(messageCallback_);
    baseServer_->setWriteCompleteCallback(writeCompleteCallback_);
    if (threadInitCallback_) {
        threadInitCallback_(0);
    }
    baseServer_->start();

    for (size_t i = 1; i < numThreads_; ++i) {
        auto thread = new std::thread(std::bind(&TcpServer::runInThread, this, i));
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while (eventLoops_[i] == nullptr) {
                cond_.wait(lock); // 等待创建子EventLoop对象成功再继续
            }
        }
        threads_.emplace_back(thread);
    }
}

void TcpServer::runInThread(size_t index) {
    EventLoop loop;
    TcpServerSingle server(&loop, local_); // 子EventLoop中的单独TcpServerSingle实例

    server.setConnectionCallback(connectionCallback_);
    server.setMessageCallback(messageCallback_);
    server.setWriteCompleteCallback(writeCompleteCallback_);

    {
        std::lock_guard<std::mutex> guard(mutex_);
        eventLoops_[index] = &loop;
        cond_.notify_one();
    }

    if (threadInitCallback_) {
        threadInitCallback_(index);
    }
    server.start();
    loop.loop();
    // 子EventLoop是栈上对象，若loop退出，意味着栈空间将回收，将指向子loop的指针置空
    {
        std::lock_guard<std::mutex> guard(mutex_);
        eventLoops_[index] = nullptr;
    }
}

