## 代码逻辑

本项目是一个基于 Reactor 模式的高性能 HTTP 服务器，采用事件驱动架构，使用 epoll 实现 I/O 多路复用。

### 架构层次

#### 1. 系统调用封装层

- **InetAddress**: 封装 `sockaddr_in` 结构体，处理 IP 地址和端口的设置与获取
- **Socket**: 封装 socket 系统调用，提供 `bind`、`listen`、`accept`、`setnonblocking` 等功能
- **utils**: 提供错误处理函数 `errif`，用于统一的错误检查和退出

#### 2. 事件驱动核心层

- **Epoll**: 
  - 封装 `epoll_create1`、`epoll_ctl`、`epoll_wait` 系统调用
  - 管理文件描述符的注册、修改和事件等待
  - 使用 `epoll_event.data.ptr` 存储 `Channel*` 指针，实现快速上下文获取

- **Channel**: 
  - 封装文件描述符（fd）和事件（events、revents）
  - 每个 fd 对应一个 Channel 对象
  - 包含回调函数 `callback`，事件触发时执行
  - 通过 `enableReading()` 注册到 epoll 监听读事件（EPOLLIN | EPOLLET）

- **EventLoop**: 
  - 事件循环核心，不断调用 `epoll_wait` 等待事件
  - 获取活跃的 Channel 列表，依次调用 `handleEvent()` 处理事件
  - 提供 `updateChannel()` 接口用于注册/更新 Channel

#### 3. 网络服务器层

- **Acceptor**: 
  - 负责接受新连接
  - 创建监听 socket，绑定地址（127.0.0.1:1234），设置为非阻塞模式
  - 创建 `acceptChannel` 监听服务器 socket 的读事件
  - 当有新连接到达时，调用 `acceptConnection()` 接受连接并触发回调

- **Connection**: 
  - 处理单个客户端连接
  - 创建 `Channel` 监听客户端 socket 的读事件
  - 实现 `echo()` 函数：读取客户端数据并回显
  - 使用非阻塞 I/O，循环读取直到 `EAGAIN` 或 `EWOULDBLOCK`
  - 连接关闭时（`read` 返回 0）调用删除回调

- **Server**: 
  - 服务器主类，管理所有连接
  - 使用 `std::map<int, Connection*>` 存储所有活跃连接（以 fd 为 key）
  - 创建 `Acceptor` 并设置新连接回调
  - 提供 `newConnection()` 创建新连接，`deleteConnection()` 删除连接

### 工作流程

1. **初始化阶段**：
   ```
   main() 
   → 创建 EventLoop 
   → 创建 Server 
   → Server 创建 Acceptor 
   → Acceptor 创建监听 socket 并注册到 EventLoop
   ```

2. **事件循环**：
   ```
   EventLoop::loop() 
   → Epoll::poll() 等待事件 
   → 返回活跃的 Channel 列表 
   → 依次调用 Channel::handleEvent() 
   → 执行注册的回调函数
   ```

3. **接受新连接**：
   ```
   客户端连接到达 
   → Acceptor 的 acceptChannel 触发 
   → 调用 Acceptor::acceptConnection() 
   → 创建新的 Socket 和 Connection 
   → Connection 注册到 EventLoop 监听读事件
   ```

4. **处理客户端数据**：
   ```
   客户端数据到达 
   → Connection 的 Channel 触发 
   → 调用 Connection::echo() 
   → 读取数据并回显 
   → 如果连接关闭，调用删除回调清理资源
   ```

### 设计模式

- **Reactor 模式**：单线程事件循环 + I/O 多路复用
- **回调机制**：使用 `std::function` 实现灵活的回调注册
- **RAII**：通过构造函数和析构函数管理资源（socket、Channel 等）
- **非阻塞 I/O**：所有 socket 设置为非阻塞模式，配合 epoll 实现高并发


## 构建
```bash
# 1. 创建构建目录
mkdir build
cd build

# 2. 运行 CMake 配置
cmake ..

# 3. 编译项目
make

# 4. 运行程序
./bin/server    # 在一个终端运行
./bin/client    # 在另一个终端运行

# 5. 清理构建文件
make clean-all
# 或者
cd ..
rm -rf build
```