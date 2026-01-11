#include "knetlib/TcpConnection.h"
#include "knetlib/EventLoop.h"
#include "knetlib/Logger.h"
#include "knetlib/utils.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cassert>
#include <cstring>

namespace {

enum ConnectState {
    kConnecting,
    kConnected,
    kDisconnecting,
    kDisconnected
};

} // anonymous namespace

TcpConnection::TcpConnection(EventLoop* loop, int sockfd, const InetAddress& local, const InetAddress& peer)
        : loop_(loop),
          sockfd_(sockfd),
          channel_(loop, sockfd_),
          state_(kConnecting),
          local_(local),
          peer_(peer),
          highWaterMark_(0)
{
    channel_.setReadCallback([this](){handleRead();});
    channel_.setWriteCallback([this](){handleWrite();});
    channel_.setCloseCallback([this](){handleClose();});
    channel_.setErrorCallback([this](){handleError();});

    TRACE("TcpConnection() {} fd={}", name().c_str(), sockfd_);
}

TcpConnection::~TcpConnection() {
    int currentState = state_.load(std::memory_order_acquire);
    // 如果连接状态不是 kDisconnected，说明连接没有被正确关闭
    // 这可能发生在 EventLoop 退出时，连接还在运行
    // 在这种情况下，我们直接关闭 socket，但记录警告
    if (currentState != kDisconnected) {
        WARN("TcpConnection::~TcpConnection() connection not properly closed, state=%d, fd=%d", 
             currentState, sockfd_);
        // 直接关闭 socket，避免资源泄漏
        if (sockfd_ != -1) {
            close(sockfd_);
        }
        // 注意：我们不设置状态为 kDisconnected，因为对象正在析构
    } else {
        TRACE("~TcpConnection() {} fd={}", name().c_str(), sockfd_);
        if (sockfd_ != -1) {
            close(sockfd_);
        }
    }
}

void TcpConnection::setMessageCallback(const MessageCallback& callback) {
    messageCallback_ = callback;
}
void TcpConnection::setWriteCompleteCallback(const WriteCompleteCallback& callback) {
    writeCompleteCallback_ = callback;
}
void TcpConnection::setHighWaterMarkCallback(const HighWaterMarkCallback& callback, size_t mark) {
    highWaterMark_ = mark;
    highWaterMarkCallback_ = callback;
}
void TcpConnection::setCloseCallback(const CloseCallback& callback) {
    closeCallback_ = callback;
}

void TcpConnection::connectEstablished() {
    int expected = kConnecting;
    assert(state_.load(std::memory_order_acquire) == kConnecting);
    state_.store(kConnected, std::memory_order_release);
    channel_.tie(shared_from_this()); // 将socketfd_的Channel和TcpConnection绑定
    channel_.enableRead(); // 打开socket的读
}
bool TcpConnection::connected() const {
    return state_.load(std::memory_order_acquire) == kConnected;
}
bool TcpConnection::disconnected() const {
    return state_.load(std::memory_order_acquire) == kDisconnected;
}

const InetAddress& TcpConnection::local() const {
    return local_;
}
const InetAddress& TcpConnection::peer() const {
    return peer_;
}
std::string TcpConnection::name() const {
    return peer_.toIpPort() + " -> " + local_.toIpPort();
}

void TcpConnection::setContext(const std::any& context) {
    context_ = context;
}
const std::any& TcpConnection::getContext() const {
    return context_;
}
std::any& TcpConnection::getContext() {
    return context_;
}

void TcpConnection::send(const std::string& data) {
    send(data.data(), data.length());
}
void TcpConnection::send(const char* data, size_t len) {
    if (state_.load(std::memory_order_acquire) != kConnected) {
        WARN("TcpConnection::send() not connected, give up send");
        return;
    }
    // 在自己所属的EventLoop中
    if (loop_->isInLoopThread()) {
        sendInLoop(data, len);
    }
    // 多Reactor，没有在自己所属的EventLoop中
    else {
        loop_->queueInLoop(
                [ptr = shared_from_this(), str = std::string(data, data+len)]()
                { ptr->sendInLoop(str);});
    }
}
void TcpConnection::send(Buffer& buffer) {
    if (state_.load(std::memory_order_acquire) != kConnected) {
        WARN("TcpConnection::send() not connected, give up send");
        return;
    }
    if (loop_->isInLoopThread()) {
        sendInLoop(buffer.peek(), buffer.readableBytes());
        buffer.retrieveAll();
    }
    else {
        loop_->queueInLoop([ptr = shared_from_this(), str = buffer.retrieveAllAsString()](){ptr->sendInLoop(str);});
    }
}

void TcpConnection::shutdown() {
    assert(state_.load(std::memory_order_acquire) != kDisconnected);
    if (stateAtomicGetAndSet(kDisconnecting) == kConnected) {
        if (loop_->isInLoopThread()) {
            shutdownInLoop();
        }
        else {
            // 成员函数第一个隐藏形参为this指针
            loop_->queueInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
        }
    }
}
void TcpConnection::forceClose() {
    if (state_.load(std::memory_order_acquire) != kDisconnected) {
        if (stateAtomicGetAndSet(kDisconnecting) != kDisconnected) {
            // 如果我们在 EventLoop 线程中，可以直接调用 forceCloseInLoop
            // 这样可以避免 queueInLoop 在 EventLoop 退出后无法执行的问题
            if (loop_->isInLoopThread()) {
                forceCloseInLoop();
            } else {
                loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
            }
        }
    }
}

void TcpConnection::stopRead() {
    loop_->runInLoop([this]() {
        if (channel_.isReading()) {
            channel_.disableRead();
        }
    });
}
void TcpConnection::startRead() {
    loop_->runInLoop([this]() {
        if (!channel_.isReading()) {
            channel_.enableRead();
        }
    });
}
bool TcpConnection::isReading() {
    return channel_.isReading();
}

const Buffer& TcpConnection::inputBuffer() const {
    return inputBuffer_;
}
const Buffer& TcpConnection::outputBuffer() const {
    return outputBuffer_;
}

void TcpConnection::handleRead() {
    loop_->assertInLoopThread();
    // 如果连接已经断开，直接返回，避免在关闭过程中处理事件
    if (state_.load(std::memory_order_acquire) == kDisconnected) {
        return;
    }
    int savedErrno;
    ssize_t n = inputBuffer_.readFd(sockfd_, &savedErrno);
    if (n == -1) {
        errno = savedErrno;
        SYSERR("TcpConnection::read()");
        handleError();
    }
    else if (n == 0) {
        handleClose();
    }
    else {
        if (messageCallback_) {
            messageCallback_(shared_from_this(), inputBuffer_);
        }
    }
}
void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if (state_.load(std::memory_order_acquire) == kDisconnected) {
        WARN("TcpConnection::handleWrite() disconnected, give up writing %zu bytes", outputBuffer_.readableBytes());
        return;
    }
    assert(outputBuffer_.readableBytes() > 0);
    assert(channel_.isWriting());
    ssize_t n = ::write(sockfd_, outputBuffer_.peek(), outputBuffer_.readableBytes());
    if (n == -1) {
        int savedErrno = errno;
        if (savedErrno != EAGAIN) {
            SYSERR("TcpConnection::write()");
            if (savedErrno == EPIPE || savedErrno == ECONNRESET) {
                handleError();
            }
        }
    }
    else {
        outputBuffer_.retrieve(static_cast<size_t>(n));
        if (outputBuffer_.readableBytes() == 0) {
            channel_.disableWrite();
            if (state_.load(std::memory_order_acquire) == kDisconnecting)
                shutdownInLoop();
            if (writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
    }
}
void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    int currentState = state_.load(std::memory_order_acquire);
    assert(currentState == kConnected || currentState == kDisconnecting);
    state_.store(kDisconnected, std::memory_order_release);
    loop_->removeChannel(&channel_);
    if (closeCallback_) {
        closeCallback_(shared_from_this());
    }
}
void TcpConnection::handleError() {
    loop_->assertInLoopThread();
    int err;
    socklen_t len = sizeof(err);
    int ret = getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &err, &len);
    if (ret != -1)
        errno = err;
    SYSERR("TcpConnection::handleError()");
    // 发生错误时关闭连接
    handleClose();
}

void TcpConnection::sendInLoop(const char *data, size_t len) {
    loop_->assertInLoopThread();
    if (state_.load(std::memory_order_acquire) == kDisconnected) {
        WARN("TcpConnection::sendInLoop() disconnected, give up send");
        return;
    }
    ssize_t n = 0;
    size_t remain = len;
    bool faultError = false;
    /**
     * 如果已经在监听EPOLLOUT事件了，说明sockfd_内核缓冲区已经是已满状态，因此就不会尝试执行write
     * 的操作，而是直接执行下面的逻辑，将待发送数据追加到outputBuffer_ 
    **/
    if (!channel_.isWriting()) {
        assert(outputBuffer_.readableBytes() == 0);
        n = ::write(sockfd_, data, len);
        if (n == -1) {
            if (errno != EAGAIN) {
                SYSERR("TcpConnection::write()");
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                    // 连接已断开，关闭连接
                    handleClose();
                }
            }
            n = 0;
        }
        else {
            remain -= static_cast<size_t>(n);
            if (remain == 0 && writeCompleteCallback_) {
                // 正常写完了，执行写完成回调
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
    }
    /**
     * 如果没有发生write错误，并且还有数据没有写完，说明sockfd_的内核缓冲区已满，只写了一部分进去，
     * 此刻将会触发高水位回调，并且注册sockfd_(channel_)的EPOLLOUT事件，当channel_中有数据出去后，
     * 内核缓冲区将有空余空间，此时将之前没写完，暂存在outputBuffer_中的数据继续向sockfd_的内核
     * 缓冲区写入
    **/
    if (!faultError && remain > 0) {
        if (highWaterMarkCallback_) {
            size_t oldLen = outputBuffer_.readableBytes();
            size_t newLen = oldLen + remain;
            if (oldLen < highWaterMark_ && newLen >= highWaterMark_)
                loop_->queueInLoop(std::bind(
                        highWaterMarkCallback_, shared_from_this(), newLen));
        }
        outputBuffer_.append(data + n, remain);
        channel_.enableWrite();
    }
}
void TcpConnection::sendInLoop(const std::string& message) {
    sendInLoop(message.data(), message.length());
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (state_.load(std::memory_order_acquire) != kDisconnected && !channel_.isWriting()) {
        if (::shutdown(sockfd_, SHUT_WR) == -1) {
            SYSERR("TcpConnection::shutdown()");
        }
    }
}
void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if (state_.load(std::memory_order_acquire) != kDisconnected) {
        handleClose();
    }
}

int TcpConnection::stateAtomicGetAndSet(int newState) {
    return state_.exchange(newState, std::memory_order_acq_rel);
}

