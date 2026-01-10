#pragma once

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class InetAddress;

// 前向声明，避免循环依赖
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer&)>;
using ErrorCallback = std::function<void()>;
using NewConnectionCallback = std::function<void(int sockfd, const InetAddress& local, const InetAddress& peer)>;
using Task = std::function<void()>;
using ThreadInitCallback = std::function<void(size_t)>;
using TimerCallback = std::function<void()>;

