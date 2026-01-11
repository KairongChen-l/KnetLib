#!/bin/bash

# ApacheBench 性能测试脚本
# 用于测试 KnetLib 网络库的 HTTP 服务器性能

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 配置参数
SERVER_PORT=8888
SERVER_BINARY=""
BUILD_DIR="build"
NUM_THREADS=1
AB_OPTIONS=""

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--port)
            SERVER_PORT="$2"
            shift 2
            ;;
        -t|--threads)
            NUM_THREADS="$2"
            shift 2
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -n|--requests)
            AB_OPTIONS="$AB_OPTIONS -n $2"
            shift 2
            ;;
        -c|--concurrency)
            AB_OPTIONS="$AB_OPTIONS -c $2"
            shift 2
            ;;
        -k|--keepalive)
            AB_OPTIONS="$AB_OPTIONS -k"
            shift
            ;;
        -h|--help)
            echo "用法: $0 [选项]"
            echo "选项:"
            echo "  -p, --port PORT          服务器端口 (默认: 8888)"
            echo "  -t, --threads N          服务器线程数 (默认: 1)"
            echo "  -b, --build-dir DIR      构建目录 (默认: build)"
            echo "  -n, --requests N         总请求数 (ApacheBench 选项)"
            echo "  -c, --concurrency N      并发数 (ApacheBench 选项)"
            echo "  -k, --keepalive          启用 HTTP Keep-Alive"
            echo "  -h, --help               显示此帮助信息"
            exit 0
            ;;
        *)
            echo "未知选项: $1"
            exit 1
            ;;
    esac
done

# 检查 ApacheBench 是否安装
if ! command -v ab &> /dev/null; then
    echo -e "${RED}错误: 未找到 ApacheBench (ab)${NC}"
    echo "请安装 ApacheBench:"
    echo "  Ubuntu/Debian: sudo apt-get install apache2-utils"
    echo "  CentOS/RHEL:   sudo yum install httpd-tools"
    exit 1
fi

# 查找服务器二进制文件
if [ -f "${BUILD_DIR}/bin/benchmark_server" ]; then
    SERVER_BINARY="${BUILD_DIR}/bin/benchmark_server"
elif [ -f "build/bin/benchmark_server" ]; then
    SERVER_BINARY="build/bin/benchmark_server"
    BUILD_DIR="build"
else
    echo -e "${RED}错误: 未找到 benchmark_server 二进制文件${NC}"
    echo "请先编译项目:"
    echo "  mkdir -p build && cd build && cmake .. && make benchmark_server"
    exit 1
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}KnetLib ApacheBench 性能测试${NC}"
echo -e "${BLUE}========================================${NC}"
echo "服务器二进制: $SERVER_BINARY"
echo "服务器端口: $SERVER_PORT"
echo "服务器线程数: $NUM_THREADS"
echo ""

# 检查端口是否被占用
if lsof -Pi :${SERVER_PORT} -sTCP:LISTEN -t >/dev/null 2>&1 ; then
    echo -e "${YELLOW}警告: 端口 ${SERVER_PORT} 已被占用${NC}"
    read -p "是否继续? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# 启动服务器（后台运行）
echo -e "${GREEN}启动性能测试服务器...${NC}"
"$SERVER_BINARY" "$SERVER_PORT" "$NUM_THREADS" > /tmp/knetlib_benchmark_server.log 2>&1 &
SERVER_PID=$!

# 等待服务器启动并验证
sleep 2

# 验证服务器是否真的在监听
for i in {1..5}; do
    if curl -s -o /dev/null -w "%{http_code}" http://localhost:${SERVER_PORT}/plain | grep -q "200"; then
        echo -e "${GREEN}服务器验证成功${NC}"
        break
    fi
    if [ $i -eq 5 ]; then
        echo -e "${RED}错误: 服务器启动后无法响应请求${NC}"
        echo "服务器日志:"
        cat /tmp/knetlib_benchmark_server.log
        kill $SERVER_PID 2>/dev/null || true
        exit 1
    fi
    sleep 1
done

# 检查服务器是否正在运行
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${RED}错误: 服务器启动失败${NC}"
    echo "服务器日志:"
    cat /tmp/knetlib_benchmark_server.log
    exit 1
fi

echo -e "${GREEN}服务器已启动 (PID: $SERVER_PID)${NC}"
echo ""

# 清理函数
cleanup() {
    echo ""
    echo -e "${YELLOW}正在停止服务器...${NC}"
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    echo -e "${GREEN}服务器已停止${NC}"
}
trap cleanup EXIT INT TERM

# 测试函数
run_benchmark() {
    local endpoint=$1
    local name=$2
    local requests=${3:-10000}
    local concurrency=${4:-100}
    local keepalive=${5:-""}
    
    echo -e "${BLUE}----------------------------------------${NC}"
    echo -e "${BLUE}测试: $name${NC}"
    echo -e "${BLUE}端点: $endpoint${NC}"
    echo -e "${BLUE}请求数: $requests${NC}"
    echo -e "${BLUE}并发数: $concurrency${NC}"
    echo -e "${BLUE}----------------------------------------${NC}"
    
    local ab_cmd="ab -s 300 -n $requests -c $concurrency"
    if [ "$keepalive" = "keepalive" ]; then
        ab_cmd="$ab_cmd -k"
    fi
    ab_cmd="$ab_cmd http://localhost:${SERVER_PORT}${endpoint}"
    
    echo "执行命令: $ab_cmd"
    echo ""
    
    $ab_cmd
    
    echo ""
    sleep 1
}

# 如果用户提供了自定义选项，只运行一次测试
if [ -n "$AB_OPTIONS" ]; then
    echo -e "${BLUE}运行自定义测试...${NC}"
    local ab_cmd="ab -s 300 $AB_OPTIONS http://localhost:${SERVER_PORT}/"
    echo "执行命令: $ab_cmd"
    echo ""
    $ab_cmd
else
    # 运行标准性能测试套件
    echo -e "${GREEN}开始性能测试套件...${NC}"
    echo ""
    
    # 测试 1: 基础性能测试（小并发）
    run_benchmark "/plain" "基础性能测试 (小并发)" 10000 10
    
    # 测试 2: 中等并发测试
    run_benchmark "/plain" "中等并发测试" 50000 50
    
    # 测试 3: 高并发测试
    run_benchmark "/plain" "高并发测试" 100000 100
    
    # 测试 4: 极高并发测试
    run_benchmark "/plain" "极高并发测试" 200000 200
    
    # 测试 5: Keep-Alive 测试（小并发）
    run_benchmark "/plain" "Keep-Alive 测试 (小并发)" 100000 10 "keepalive"
    
    # 测试 6: Keep-Alive 测试（高并发）
    run_benchmark "/plain" "Keep-Alive 测试 (高并发)" 200000 100 "keepalive"
    
    # 测试 7: HTML 响应测试
    run_benchmark "/" "HTML 响应测试" 50000 50
    
    # 测试 8: JSON 响应测试
    run_benchmark "/json" "JSON 响应测试" 50000 50
fi

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}所有测试完成${NC}"
echo -e "${GREEN}========================================${NC}"

