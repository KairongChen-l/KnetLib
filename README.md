## 代码逻辑

本项目是一个基于 **主从 Reactor 模式** 的高性能网络服务器，采用事件驱动架构，使用 epoll 实现 I/O 多路复用，并结合线程池实现多线程并发处理。

### 架构层次

#### 1. 系统调用封装层

- **InetAddress**: 封装 `sockaddr_in` 结构体，处理 IP 地址和端口的设置与获取
- **Socket**: 封装 socket 系统调用，提供 `bind`、`listen`、`accept`、`connect`、`setnonblocking` 等功能
- **Buffer**: 封装数据缓冲区，使用 `std::string` 实现，提供 `append`、`clear`、`c_str` 等接口
- **utils**: 提供错误处理函数 `errif`，用于统一的错误检查和退出

#### 2. 事件驱动核心层

- **Epoll**: 
  - 封装 `epoll_create1`、`epoll_ctl`、`epoll_wait` 系统调用
  - 管理文件描述符的注册、修改、删除和事件等待
  - 使用 `epoll_event.data.ptr` 存储 `Channel*` 指针，实现快速上下文获取
  - 提供 `updateChannel()` 和 `deleteChannel()` 接口

- **Channel**: 
  - 封装文件描述符（fd）和事件（events、ready）
  - 每个 fd 对应一个 Channel 对象
  - 包含读/写回调函数（`readCallback`、`writeCallback`）
  - 支持边缘触发模式（EPOLLET）
  - 可选择是否使用线程池执行回调（`useThreadPool`）
  - 通过 `enableRead()` 注册到 epoll 监听读事件

- **EventLoop**: 
  - 事件循环核心，不断调用 `epoll_wait` 等待事件
  - 每个 EventLoop 拥有独立的 Epoll 和 ThreadPool
  - 获取活跃的 Channel 列表，依次调用 `handleEvent()` 处理事件
  - 提供 `updateChannel()` 接口用于注册/更新 Channel
  - 提供 `addThread()` 接口将任务提交到线程池

#### 3. 网络服务器层

- **Acceptor**: 
  - 负责接受新连接
  - 创建监听 socket，绑定地址（127.0.0.1:1234），设置为非阻塞模式
  - 创建 `acceptChannel` 监听服务器 socket 的读事件
  - 不使用线程池（`setUseThreadPool(false)`），在主线程同步处理
  - 当有新连接到达时，调用 `acceptConnection()` 接受连接并触发回调

- **Connection**: 
  - 处理单个客户端连接
  - 创建 `Channel` 监听客户端 socket 的读事件，使用边缘触发模式
  - 使用线程池处理回调（`setUseThreadPool(true)`）
  - 实现 `echo()` 函数：使用非阻塞 I/O 循环读取数据，读取完成后回显
  - 使用 `Buffer` 存储读取的数据
  - 连接关闭时（`read` 返回 0）调用删除回调
  - 提供 `send()` 函数用于发送数据（处理 EAGAIN 情况）

- **Server**: 
  - 服务器主类，实现主从 Reactor 模式
  - **主 Reactor（mainReactor）**：在主线程运行，负责接受新连接
  - **从 Reactors（subReactors）**：多个子事件循环，数量等于 CPU 核心数
  - **线程池（thpool）**：用于运行子事件循环和处理任务
  - 使用 `std::map<int, Connection*>` 存储所有活跃连接（以 fd 为 key）
  - 使用负载均衡算法（`fd % subReactors.size()`）将新连接分配给子 Reactor
  - 提供 `newConnection()` 创建新连接，`deleteConnection()` 删除连接

#### 4. 线程池层

- **ThreadPool**: 
  - 使用模板实现，支持任意可调用对象和参数
  - 返回 `std::future` 用于获取异步执行结果
  - 使用条件变量实现任务队列的生产者-消费者模式
  - 支持优雅关闭（等待所有任务完成）

### 工作流程

1. **初始化阶段**：
   ```
   main() 
   → 创建主 EventLoop（mainReactor）
   → 创建 Server 
   → Server 创建 Acceptor（注册到 mainReactor）
   → Server 创建线程池（大小为 CPU 核心数）
   → Server 创建多个子 EventLoop（subReactors）
   → 将子 EventLoop 的 loop() 函数提交到线程池运行
   ```

2. **主事件循环**（主线程）：
   ```
   mainReactor::loop() 
   → Epoll::poll() 等待事件 
   → 返回活跃的 Channel 列表 
   → 依次调用 Channel::handleEvent() 
   → Acceptor 的 acceptChannel 触发，接受新连接
   ```

3. **接受新连接**：
   ```
   客户端连接到达 
   → Acceptor 的 acceptChannel 在主线程触发 
   → 调用 Acceptor::acceptConnection() 
   → 创建新的 Socket 和 Connection 
   → 使用负载均衡算法选择子 Reactor（fd % subReactors.size()）
   → Connection 注册到选中的子 Reactor，监听读事件
   ```

4. **子事件循环**（线程池中的线程）：
   ```
   子 Reactor::loop() 在线程池中运行
   → Epoll::poll() 等待事件
   → 返回活跃的 Channel 列表
   → 依次调用 Channel::handleEvent()
   ```

5. **处理客户端数据**：
   ```
   客户端数据到达 
   → Connection 的 Channel 在子 Reactor 中触发 
   → Channel::handleEvent() 检测到 useThreadPool == true
   → 将 Connection::echo() 回调提交到线程池
   → 线程池中的线程执行 echo()
   → 使用非阻塞 I/O 循环读取数据到 Buffer
   → 读取完成后回显数据
   → 如果连接关闭，调用删除回调清理资源
   ```

### 架构特点

#### 主从 Reactor 模式

- **主 Reactor**：单线程，专门处理新连接，避免连接建立成为瓶颈
- **从 Reactors**：多线程，每个线程运行一个事件循环，处理已建立的连接
- **负载均衡**：使用简单的取模算法分配连接，后续可优化为更复杂的负载均衡策略

#### 线程池机制

- **两层线程池**：
  1. 第一层：运行子事件循环（subReactors）
  2. 第二层：处理具体的业务逻辑（Connection 的回调）
- **灵活配置**：Channel 可以选择是否使用线程池执行回调
  - Acceptor 不使用线程池（主线程同步处理，保证连接建立的及时性）
  - Connection 使用线程池（避免阻塞事件循环）

#### 非阻塞 I/O + 边缘触发

- 所有 socket 设置为非阻塞模式
- 使用 EPOLLET（边缘触发），内核只通知一次
- 在回调中必须一次性读取完所有数据，直到 `EAGAIN` 或 `EWOULDBLOCK`

### 设计模式

- **主从 Reactor 模式**：主线程处理连接建立，工作线程处理连接 I/O
- **线程池模式**：复用线程，减少线程创建和销毁的开销
- **回调机制**：使用 `std::function` 实现灵活的回调注册
- **RAII**：通过构造函数和析构函数管理资源（socket、Channel、Buffer 等）
- **非阻塞 I/O**：所有 socket 设置为非阻塞模式，配合 epoll 实现高并发

### 已知问题

- **多线程安全问题**：`Connection::deleteConnectionCallBack` 在多线程环境下可能存在竞态条件，需要后续优化
- **连接删除**：当前实现中直接删除 Connection 可能导致段错误，需要实现线程安全的延迟删除机制

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
./bin/server    # 在一个终端运行服务器
./bin/client    # 在另一个终端运行客户端

# 5. 清理构建文件
make clean-all

# 6. 运行测试
make ThreadPoolTest    # 编译线程池测试
./bin/ThreadPoolTest   # 运行测试

make test              # 编译压力测试
./bin/test             # 运行压力测试（默认 100 线程，每个 100 消息）
./bin/test -t 50 -m 200  # 自定义参数：50 线程，每个 200 消息
```
