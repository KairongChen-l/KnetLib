#include "Acceptor.h"
#include "EventLoop.h"
#include "utils.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace {

int createSocket() {
    int ret = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (ret == -1) {
        errif(true, "Acceptor createSocket");
    }
    return ret;
}

} // anonymous namespace

Acceptor::Acceptor(EventLoop* loop, const InetAddress& local)
        : listening_(false),
          loop_(loop),
          acceptfd_(createSocket()),
          acceptChannel_(loop, acceptfd_),
          local_(local)
{
    int on = 1;
    int ret = setsockopt(acceptfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret == -1) {
        errif(true, "Acceptor setsockopt SO_REUSEADDR");
    }
    ret = setsockopt(acceptfd_, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    if (ret == -1) {
        errif(true, "Acceptor setsockopt SO_REUSEPORT");
    }
    ret = bind(acceptfd_, local.getSockaddr(), local.getSocklen());
    if (ret == -1) {
        errif(true, "Acceptor bind");
    }
}

Acceptor::~Acceptor() {
    if (acceptfd_ != -1) {
        close(acceptfd_);
    }
}

bool Acceptor::listening() const {
    return listening_;
}

void Acceptor::listen() {
    loop_->assertInLoopThread();
    int ret = ::listen(acceptfd_, SOMAXCONN);
    if (ret == -1) {
        errif(true, "Acceptor listen fatal");
    }
    listening_ = true;
    acceptChannel_.setReadCallback([this](){handleRead();}); // 当有连接请求到来时，交由handleRead处理
    acceptChannel_.enableRead();
}

void Acceptor::setNewConnectionCallback(const NewConnectionCallback& callback) {
    newConnectionCallback_ = callback;
}

void Acceptor::handleRead() {
    loop_->assertInLoopThread();

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    int sockfd = ::accept4(acceptfd_, reinterpret_cast<sockaddr*>(&addr), &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (sockfd == -1) {
        int savedErrno = errno;
        // 错误处理
        switch (savedErrno) {
            case ECONNABORTED: // connection aborted
            case EMFILE: // 文件描述符用完了
                // 可以在这里添加日志输出
                break;
            default:
                errif(true, "unexpected accept4() error");
        }
        return;
    }

    if (newConnectionCallback_) {
        InetAddress peer;
        peer.setAddress(addr);
        newConnectionCallback_(sockfd, local_, peer);
    }
    else {
        ::close(sockfd);
    }
}
