#!/bin/bash

# KnetLib 完整测试脚本
# 运行所有测试并生成报告

set -e  # 遇到错误立即退出

echo "=========================================="
echo "KnetLib 完整测试套件"
echo "=========================================="
echo ""

# 检查是否在 build 目录
if [ ! -f "CMakeCache.txt" ]; then
    echo "❌ 错误: 请在 build 目录中运行此脚本"
    echo "使用方法:"
    echo "  cd build"
    echo "  cmake .."
    echo "  make"
    echo "  ../test/run_all_tests.sh"
    exit 1
fi

# 检查是否已编译
if [ ! -d "bin" ]; then
    echo "❌ 错误: 请先编译项目"
    echo "运行: make -j\$(nproc)"
    exit 1
fi

echo "📦 测试组件列表："
echo "  1. Buffer"
echo "  2. Channel"
echo "  3. EventLoop"
echo "  4. Logger"
echo "  5. Timer"
echo "  6. TimerQueue"
echo "  7. InetAddress"
echo "  8. TcpConnection"
echo "  9. Epoll"
echo ""

# 统计变量
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试函数
run_test() {
    local test_name=$1
    local test_binary="bin/${test_name}"
    
    if [ ! -f "$test_binary" ]; then
        echo "⚠️  跳过: $test_name (未找到)"
        return
    fi
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "🧪 运行测试: $test_name"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if ./$test_binary --gtest_color=yes; then
        echo "✅ $test_name: 通过"
        ((PASSED_TESTS++))
    else
        echo "❌ $test_name: 失败"
        ((FAILED_TESTS++))
    fi
    
    ((TOTAL_TESTS++))
    echo ""
}

# 运行所有测试
echo "开始运行测试..."
echo ""

run_test "BufferTest"
run_test "ChannelTest"
run_test "EventLoopTest"
run_test "LoggerTest"
run_test "TimerTest"
run_test "TimerQueueTest"
run_test "InetAddressTest"
run_test "TcpConnectionTest"
run_test "EpollTest"

# 使用 CTest 运行（如果可用）
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 使用 CTest 运行所有测试"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if command -v ctest &> /dev/null; then
    ctest --output-on-failure --verbose || true
else
    echo "⚠️  CTest 不可用，跳过"
fi

# 输出统计
echo ""
echo "=========================================="
echo "📈 测试统计"
echo "=========================================="
echo "总测试数: $TOTAL_TESTS"
echo "通过: $PASSED_TESTS"
echo "失败: $FAILED_TESTS"
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo "🎉 所有测试通过！"
    exit 0
else
    echo "❌ 有 $FAILED_TESTS 个测试失败"
    exit 1
fi

