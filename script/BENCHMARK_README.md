# ApacheBench 性能测试指南

本目录包含使用 ApacheBench (ab) 对 KnetLib 网络库进行性能测试的工具。

## 前置要求

1. **安装 ApacheBench**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install apache2-utils
   
   # CentOS/RHEL
   sudo yum install httpd-tools
   
   # macOS
   brew install httpd
   ```

2. **编译性能测试服务器**
   ```bash
   mkdir -p build
   cd build
   cmake ..
   make benchmark_server
   ```

## 使用方法

### 基本用法

运行完整的性能测试套件（默认配置）：
```bash
./scripts/benchmark_ab.sh
```

### 自定义选项

```bash
# 指定服务器端口和线程数
./scripts/benchmark_ab.sh -p 8888 -t 4

# 运行自定义测试（指定请求数和并发数）
./scripts/benchmark_ab.sh -n 50000 -c 100

# 启用 HTTP Keep-Alive
./scripts/benchmark_ab.sh -n 100000 -c 50 -k

# 查看所有选项
./scripts/benchmark_ab.sh --help
```

### 命令行参数

- `-p, --port PORT`: 服务器端口（默认: 8888）
- `-t, --threads N`: 服务器工作线程数（默认: 1）
- `-b, --build-dir DIR`: 构建目录（默认: build）
- `-n, --requests N`: 总请求数（ApacheBench 选项）
- `-c, --concurrency N`: 并发连接数（ApacheBench 选项）
- `-k, --keepalive`: 启用 HTTP Keep-Alive
- `-h, --help`: 显示帮助信息

## 测试端点

性能测试服务器提供以下 HTTP 端点：

- `GET /` 或 `GET /index.html` - HTML 响应
- `GET /hello` - Hello 页面
- `GET /plain` - 纯文本响应（推荐用于性能测试）
- `GET /json` - JSON 响应

## 标准测试套件

如果不指定自定义选项，脚本会运行以下标准测试：

1. **基础性能测试** - 10,000 请求，10 并发
2. **中等并发测试** - 50,000 请求，50 并发
3. **高并发测试** - 100,000 请求，100 并发
4. **极高并发测试** - 200,000 请求，200 并发
5. **Keep-Alive 测试（小并发）** - 100,000 请求，10 并发，Keep-Alive
6. **Keep-Alive 测试（高并发）** - 200,000 请求，100 并发，Keep-Alive
7. **HTML 响应测试** - 50,000 请求，50 并发
8. **JSON 响应测试** - 50,000 请求，50 并发

## 理解测试结果

ApacheBench 会输出以下关键指标：

- **Requests per second**: 每秒处理的请求数（QPS），越高越好
- **Time per request**: 每个请求的平均处理时间，越低越好
- **Transfer rate**: 数据传输速率
- **Connection Times**: 连接、处理、等待时间统计
- **Percentage of requests served within a certain time**: 响应时间分布

## 性能优化建议

1. **调整线程数**: 使用 `-t` 参数测试不同线程数对性能的影响
2. **启用 Keep-Alive**: 使用 `-k` 参数测试 HTTP Keep-Alive 的效果
3. **测试不同端点**: 不同响应大小的端点性能可能不同
4. **系统调优**: 
   - 增加文件描述符限制: `ulimit -n 65535`
   - 调整 TCP 参数（如 `net.core.somaxconn`）

## 示例输出

```
========================================
KnetLib ApacheBench 性能测试
========================================
服务器二进制: build/bin/benchmark_server
服务器端口: 8888
服务器线程数: 1

启动性能测试服务器...
服务器已启动 (PID: 12345)

----------------------------------------
测试: 基础性能测试 (小并发)
端点: /plain
请求数: 10000
并发数: 10
----------------------------------------
执行命令: ab -n 10000 -c 10 http://localhost:8888/plain

This is ApacheBench, Version 2.3 <$Revision: 1903618 $>
...
Requests per second:    12345.67 [#/sec] (mean)
Time per request:       0.810 [ms] (mean)
...
```

## 注意事项

1. 性能测试会占用大量系统资源，建议在专用测试环境中运行
2. 测试结果受系统负载、网络状况等因素影响
3. 建议多次运行测试并取平均值以获得更准确的结果
4. 确保测试期间没有其他程序占用测试端口

