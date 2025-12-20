
## `bind()` 系统调用的函数签名要求

`bind()` 的函数签名是：

```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

它接受的是通用的 `struct sockaddr*`，而不是具体的 `struct sockaddr_in*`。


### 1. 支持多种地址族

`sockaddr` 是通用地址结构，`sockaddr_in` 是 IPv4 专用结构。系统需要支持：
- IPv4：`sockaddr_in`
- IPv6：`sockaddr_in6`
- Unix domain socket：`sockaddr_un`
- 其他地址族

通过统一的 `sockaddr*` 接口，一个函数可以处理多种地址类型。

### 2. 内存布局兼容

```c
// 通用地址结构（16字节）
struct sockaddr {
    sa_family_t sa_family;    // 地址族（2字节）
    char        sa_data[14];  // 地址数据（14字节）
};

// IPv4 地址结构（16字节）
struct sockaddr_in {
    sa_family_t    sin_family;   // 地址族 = AF_INET（2字节）
    in_port_t      sin_port;      // 端口号（2字节）
    struct in_addr sin_addr;      // IP地址（4字节）
    char           sin_zero[8];    // 填充（8字节）
};
```

两者大小相同（16字节），且第一个字段都是 `sa_family`，因此可以安全转换。

### 3. 代码示例

```24:28:30daysCppWebServer/code/day08/src/Socket.cpp
void Socket::bind(InetAddress *_addr){
    struct sockaddr_in addr = _addr->getAddr();
    socklen_t addr_len = _addr->getAddr_len();
    errif(::bind(fd, (sockaddr*)&addr, addr_len) == -1, "socket bind error");
    _addr->setInetAddr(addr, addr_len);
}
```

- `addr` 是 `struct sockaddr_in` 类型
- `::bind()` 需要 `struct sockaddr*` 类型
- 通过 `(sockaddr*)&addr` 进行类型转换

### 4. 同样的模式在其他地方也使用

```38:45:30daysCppWebServer/code/day08/src/Socket.cpp
int Socket::accept(InetAddress *_addr){
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    bzero(&addr, sizeof(addr));
    int clnt_sockfd = ::accept(fd, (sockaddr*)&addr, &addr_len);
    errif(clnt_sockfd == -1, "socket accept error");
    _addr->setInetAddr(addr, addr_len);
    return clnt_sockfd;
}
```
`accept()` 也使用相同的转换模式。

- 系统调用需要统一的 `sockaddr*` 接口以支持多种地址族
- `sockaddr_in` 与 `sockaddr` 内存布局兼容，可以安全转换
- 这是 C 语言中常见的多态设计模式

**类比**：就像 USB 接口可以连接鼠标、键盘、U盘等不同设备，`sockaddr*` 可以指向不同类型的地址结构。


## `fcntl` 函数简介

`fcntl`（file control）用于控制文件描述符的属性。

### 函数原型

```c
#include <fcntl.h>
int fcntl(int fd, int cmd, ... /* arg */);
```

参数：
- `fd`：文件描述符
- `cmd`：操作命令
- `...`：根据命令不同，可能需要额外参数

## 代码解析

```34:36:30daysCppWebServer/code/day08/src/Socket.cpp
void Socket::setnonblocking(){
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}
```

这行代码分两步：

### 步骤1：获取当前文件描述符的标志

```c
fcntl(fd, F_GETFL)
```

- `F_GETFL`：获取文件描述符的标志位
- 返回当前标志（如 `O_RDONLY`、`O_WRONLY`、`O_RDWR` 等）

### 步骤2：设置新的标志（保留原有标志，添加非阻塞标志）

```c
fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)
```

- `F_SETFL`：设置文件描述符的标志位
- `fcntl(fd, F_GETFL) | O_NONBLOCK`：先获取当前标志，再用按位或添加 `O_NONBLOCK`

## 为什么要这样写？

### 错误写法（会覆盖原有标志）

```c
// ❌ 错误：这会丢失原有的标志位
fcntl(fd, F_SETFL, O_NONBLOCK);
```

### 正确写法（保留原有标志）

```c
// ✅ 正确：先获取，再按位或，最后设置
int flags = fcntl(fd, F_GETFL);      // 获取当前标志
flags |= O_NONBLOCK;                 // 添加非阻塞标志
fcntl(fd, F_SETFL, flags);           // 设置新标志

// 或者一行写完（代码中的写法）
fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
```

## 常用 `fcntl` 命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `F_GETFL` | 获取文件描述符标志 | `int flags = fcntl(fd, F_GETFL);` |
| `F_SETFL` | 设置文件描述符标志 | `fcntl(fd, F_SETFL, flags \| O_NONBLOCK);` |
| `F_GETFD` | 获取文件描述符标志（close-on-exec） | `int fd_flags = fcntl(fd, F_GETFD);` |
| `F_SETFD` | 设置文件描述符标志 | `fcntl(fd, F_SETFD, FD_CLOEXEC);` |
| `F_DUPFD` | 复制文件描述符 | `int new_fd = fcntl(fd, F_DUPFD, 0);` |

## 常用标志位

| 标志 | 说明 |
|------|------|
| `O_NONBLOCK` | 非阻塞模式 |
| `O_APPEND` | 追加模式 |
| `O_ASYNC` | 异步 I/O |
| `O_DIRECT` | 直接 I/O |
| `O_SYNC` | 同步写入 |

## 非阻塞模式的作用

设置 `O_NONBLOCK` 后：
- `read()`、`write()`、`accept()` 等不会阻塞
- 如果没有数据可读，立即返回 `-1`，并设置 `errno` 为 `EAGAIN` 或 `EWOULDBLOCK`
- 适合事件驱动的高并发服务器

## 完整示例

```c
#include <fcntl.h>
#include <unistd.h>

// 设置非阻塞
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        // 错误处理
        return;
    }
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

// 取消非阻塞
void set_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    flags &= ~O_NONBLOCK;  // 清除非阻塞标志
    fcntl(fd, F_SETFL, flags);
}
```

## 总结

这行代码的作用：
1. 获取文件描述符的当前标志位
2. 用按位或添加 `O_NONBLOCK` 标志
3. 设置新的标志位

这样既设置了非阻塞模式，又保留了原有的其他标志位。这是设置非阻塞 I/O 的标准做法。