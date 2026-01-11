#pragma once

#include "noncopyable.h"
#include <functional>
#include <memory>
#include <cstdint>
#include <sys/epoll.h>

class EventLoop;

class Channel : noncopyable {

private:
    using ReadCallback  = std::function<void()>;
    using WriteCallback = std::function<void()>;
    using CloseCallback = std::function<void()>;
    using ErrorCallback = std::function<void()>;

public:
    Channel(EventLoop* loop, int fd);
    ~Channel();

public:
    void setReadCallback(const ReadCallback& callback);
    void setWriteCallback(const WriteCallback& callback);
    void setCloseCallback(const CloseCallback& callback);
    void setErrorCallback(const ErrorCallback& callback);

    void handleEvents();
    int fd() const;
    bool isNoneEvents() const;
    unsigned events() const;
    void setRevents(unsigned);
    void tie(const std::shared_ptr<void>& obj);

    void enableRead();
    void enableWrite();
    void disableRead();
    void disableWrite();
    void disableAll();

    bool isReading() const;
    bool isWriting() const;

    // 兼容旧 API（将在阶段二重构时移除）
    void useET();  // 启用边缘触发模式
    void setUseThreadPool(bool use = true);  // 已废弃，保留以兼容旧代码

public:
    bool pooling;  // 是否在 epoll 中

private:
    EventLoop* loop_;
    int fd_;
    std::weak_ptr<void> tie_;
    bool tied_;
    unsigned events_;
    unsigned revents_;
    bool handlingEvents_;

    ReadCallback readCallback_;
    WriteCallback writeCallback_;
    CloseCallback closeCallback_;
    ErrorCallback errorCallback_;

private:
    void update();
    void remove();

    void handleEventWithGuard();

};
