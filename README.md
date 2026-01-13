# KnetLib

一个基于 Reactor 模式的高性能 C++ 网络库，采用事件驱动架构，支持高并发网络编程。

## 特性

- **事件驱动架构**：基于 epoll 的 I/O 多路复用，高效处理大量并发连接
- **主从 Reactor 模式**：主线程处理连接建立，工作线程处理 I/O 事件，职责分离
- **非阻塞 I/O**：所有 socket 设置为非阻塞模式，配合 epoll 实现高并发
- **线程安全**：One Loop Per Thread 设计，支持跨线程任务调度
- **定时器功能**：基于 timerfd 的高精度定时器，支持一次性或周期性任务
- **零拷贝优化**：Buffer 使用 readv 减少内存拷贝，提高性能
- **RAII 资源管理**：自动管理资源生命周期，避免内存泄漏

## 技术栈

- C++17
- Linux epoll
- eventfd / timerfd
- 线程池
- CMake 构建系统

## 架构概览

KnetLib 采用分层架构设计：

```
┌──────────────────────────────────────────
│         应用层 (TcpServer/TcpClient)   
├───────────────────────────────────────────
│         网络层 (TcpConnection)         
├──────────────────────────────────────────
│   事件驱动层 (EventLoop/Channel/Epoll)  
├───────────────────────────────────────────
│   系统调用封装层 (Socket/Buffer/Utils)   
└───────────────────────────────────────────
```

### 核心组件

- **EventLoop**：事件循环核心，不断调用 epoll_wait 等待事件
- **Channel**：封装文件描述符和事件，管理回调函数
- **Epoll**：封装 epoll 系统调用，管理文件描述符的注册和事件等待
- **Buffer**：高效的字节缓冲区，支持零拷贝读取
- **TcpServer**：实现主从 Reactor 模式的服务器
- **TcpConnection**：管理单个 TCP 连接的生命周期
- **EventLoopThread**：封装线程和 EventLoop 的生命周期
- **EventLoopThreadPool**：管理多个工作线程，提供连接分配
- **TimerQueue**：基于 timerfd 的定时器管理

### 主从 Reactor 模式

KnetLib 实现了标准的主从 Reactor 模式，符合 muduo 等成熟网络库的设计：

- **主 Reactor**：在主线程运行，只有一个 Acceptor 负责 accept 新连接
- **从 Reactors**：在工作线程运行，处理已建立连接的 I/O 事件
- **连接分配**：使用轮询（Round-Robin）算法将新连接分配到不同的工作线程
- **线程管理**：通过 `EventLoopThread` 和 `EventLoopThreadPool` 管理线程生命周期

**架构图**:
```
主线程: EventLoop + Acceptor (监听端口) → accept 后分配给子线程
子线程1: EventLoop (处理分配的连接)
子线程2: EventLoop (处理分配的连接)
...
```

**优势**:
- 应用层控制连接分配，支持自定义负载均衡策略
- 不依赖 Linux 特定的 SO_REUSEPORT 特性
- 符合标准设计模式，易于理解和维护

## 快速开始

### 构建要求

- Linux 系统
- C++17 编译器（GCC 7+ 或 Clang 5+）
- CMake 3.10+

### 编译

```bash
# 创建构建目录
mkdir build && cd build

# 运行 CMake 配置
cmake ..

# 编译项目
make -j

# 运行示例程序
./bin/server    # 在一个终端运行服务器
./bin/client    # 在另一个终端运行客户端
```

### 运行测试

```bash
# 编译所有测试
cd build
cmake ..
make -j

# 运行所有测试
ctest --output-on-failure

# 运行特定测试
./bin/EventLoopThreadTest
./bin/EventLoopThreadPoolTest
./bin/TcpServerTest

# 运行集成测试
./bin/test_network              # 网络集成测试
./bin/test_network -t 50 -m 200  # 自定义参数：50 线程，每个 200 消息
```

## 使用示例

### 服务器示例

```cpp
#include "TcpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"

int main() {
    EventLoop loop;
    InetAddress listenAddr(8888);
    TcpServer server(&loop, listenAddr);

    // 设置连接回调
    server.setConnectionCallback([](const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            INFO("New connection: {}", conn->name());
        } else {
            INFO("Connection closed: {}", conn->name());
        }
    });

    // 设置消息回调
    server.setMessageCallback([](const TcpConnectionPtr& conn, 
                                 Buffer* buf, 
                                 Timestamp time) {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);  // Echo 回显
    });

    // 设置工作线程数
    server.setNumThread(4);
    
    // 启动服务器
    server.start();
    loop.loop();
    
    return 0;
}
```

### 客户端示例

```cpp
#include "TcpClient.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"

int main() {
    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 8888);
    TcpClient client(&loop, serverAddr);

    // 设置连接回调
    client.setConnectionCallback([](const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            conn->send("Hello Server");
        }
    });

    // 设置消息回调
    client.setMessageCallback([](const TcpConnectionPtr& conn, 
                                Buffer* buf, 
                                Timestamp time) {
        std::string msg = buf->retrieveAllAsString();
        INFO("Received: {}", msg);
    });

    // 连接服务器
    client.connect();
    loop.loop();
    
    return 0;
}
```

## 项目结构

```
KnetLib/
├── src/                      # 源代码
│   ├── EventLoop.*           # 事件循环
│   ├── EventLoopThread.*     # 线程封装
│   ├── EventLoopThreadPool.* # 线程池
│   ├── Channel.*             # 文件描述符封装
│   ├── Epoll.*               # epoll 封装
│   ├── TcpServer.*           # 服务器实现（主从 Reactor）
│   ├── TcpConnection.*        # 连接管理
│   ├── Buffer.*               # 缓冲区
│   └── ...
├── test/                      # 测试代码
│   ├── EventLoopThreadTest.cpp
│   ├── EventLoopThreadPoolTest.cpp
│   ├── TcpServerTest.cpp
│   └── ...
├── examples/                  # 示例代码
├── build/                     # 构建目录
└── CMakeLists.txt             # CMake 配置文件
```

## 设计特点

### One Loop Per Thread

每个 EventLoop 只在一个线程中运行，通过线程 ID 检查确保线程一致性，避免跨线程直接操作，减少锁的使用。

### 跨线程调用

通过任务队列和 wakeup 机制实现跨线程调用。使用 eventfd 创建 wakeupfd，其他线程写入数据唤醒阻塞在 epoll_wait 的线程。

### 生命周期管理

使用 `std::shared_ptr` 管理 TcpConnection 生命周期，通过 Channel 的 tie 机制确保对象在回调执行期间不被析构。

### 非阻塞 I/O

所有 socket 设置为非阻塞模式，配合 epoll 实现高并发。正确处理 EAGAIN 和部分读写情况。

## 性能优化

- **零拷贝读取**：使用 readv 一次读取多个缓冲区
- **减少系统调用**：批量处理事件和任务
- **避免锁竞争**：One Loop Per Thread，每个连接绑定固定线程
- **事件驱动**：只在有事件时处理，减少 CPU 占用

## 测试覆盖

项目包含完善的单元测试和集成测试：

- ✅ EventLoopThread: 3/3 测试通过
- ✅ EventLoopThreadPool: 5/5 测试通过
- ✅ TcpServer: 7/7 测试通过
- ✅ 其他组件测试: 全部通过

运行 `ctest` 查看完整测试报告。

## 性能特性
- **低延迟**: 事件驱动架构，响应迅速
- **低开销**: One Loop Per Thread，减少锁竞争
- **可扩展**: 支持动态调整工作线程数

## 参考

本项目参考了以下优秀项目：

- [muduo](https://github.com/chenshuo/muduo) - 一个优秀的 C++ 网络库

## 更新日志

### v1.1.0 (2025-01-13)
- ✅ 实现标准主从 Reactor 模式
- ✅ 添加 EventLoopThread 和 EventLoopThreadPool
- ✅ 移除 SO_REUSEPORT 依赖
- ✅ 完善测试覆盖

