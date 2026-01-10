# KnetLib 测试目录

本目录包含 KnetLib 网络库的所有单元测试。

## 📁 测试文件

| 测试文件 | 测试组件 | 描述 |
|---------|---------|------|
| `BufferTest.cpp` | Buffer | 测试缓冲区操作 |
| `ChannelTest.cpp` | Channel | 测试事件通道 |
| `EventLoopTest.cpp` | EventLoop | 测试事件循环 |
| `LoggerTest.cpp` | Logger | 测试日志系统 |
| `TimerTest.cpp` | Timer | 测试定时器 |
| `TimerQueueTest.cpp` | TimerQueue | 测试定时器队列 |
| `InetAddressTest.cpp` | InetAddress | 测试网络地址 |
| `TcpConnectionTest.cpp` | TcpConnection | 测试 TCP 连接 |
| `EpollTest.cpp` | Epoll | 测试 Epoll 封装 |

## 🚀 快速开始

### 运行所有测试

```bash
cd build
make
../test/run_all_tests.sh
```

### 运行单个测试

```bash
./bin/BufferTest
```

### 使用 CTest

```bash
ctest --output-on-failure
```

## 📝 添加新测试

1. 在 `test/` 目录创建新的测试文件
2. 在 `CMakeLists.txt` 中添加测试目标
3. 使用 `add_knetlib_test()` 函数

示例：

```cpp
// test/NewComponentTest.cpp
#include <gtest/gtest.h>
#include "NewComponent.h"

TEST(NewComponentTest, BasicTest) {
    NewComponent comp;
    EXPECT_TRUE(comp.isValid());
}
```

在 CMakeLists.txt 中：

```cmake
add_knetlib_test(NewComponentTest)
```

## 🔍 测试最佳实践

1. **测试隔离**: 每个测试应该独立，不依赖其他测试
2. **清理资源**: 在 TearDown 中清理所有资源
3. **断言使用**: 
   - `EXPECT_*` - 测试失败时继续执行
   - `ASSERT_*` - 测试失败时立即停止
4. **命名规范**: 使用描述性的测试名称

## 📚 参考

- [GoogleTest 文档](https://google.github.io/googletest/)
- [TESTING_GUIDE.md](../TESTING_GUIDE.md) - 完整测试指南

