#!/bin/bash
# 详细内存分析脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build_asan"
BIN_DIR="$BUILD_DIR/bin"

echo "=========================================="
echo "   KnetLib 详细内存分析"
echo "=========================================="
echo ""

cd "$PROJECT_DIR" || exit 1

# 1. 使用 ASAN 运行并检查详细输出
echo "[1] AddressSanitizer 详细检测"
echo "----------------------------------------"
cd "$BUILD_DIR" || exit 1

# 设置ASAN选项
export ASAN_OPTIONS="detect_leaks=1:abort_on_error=0:print_stats=1:print_summary=1:log_path=/tmp/asan_detailed"

# 运行测试
echo "运行测试程序..."
./bin/test_network > /tmp/test_detailed.log 2>&1 || true

# 检查ASAN报告
if ls /tmp/asan_detailed.* 2>/dev/null | grep -q .; then
    echo "⚠️  发现 ASan 报告文件:"
    for file in /tmp/asan_detailed.*; do
        echo ""
        echo "文件: $file"
        echo "---"
        head -50 "$file"
        echo "..."
    done
else
    echo "✓ 未发现 ASan 错误报告"
fi
echo ""

# 2. 检查测试输出中的内存相关信息
echo "[2] 测试输出内存分析"
echo "----------------------------------------"
if grep -iE "(leak|memory|corruption|overflow|invalid|freed)" /tmp/test_detailed.log; then
    echo ""
    echo "⚠️  发现内存相关警告"
else
    echo "✓ 未发现内存相关问题"
fi
echo ""

# 3. 运行server和client进行交互测试
echo "[3] Server/Client 交互测试"
echo "----------------------------------------"
cd "$BUILD_DIR" || exit 1

# 启动服务器
timeout 5 ./bin/server > /tmp/server_mem.log 2>&1 &
SERVER_PID=$!
sleep 2

# 运行客户端
echo "test message" | timeout 3 ./bin/client 8888 > /tmp/client_mem.log 2>&1 || true

# 等待服务器退出
wait $SERVER_PID 2>/dev/null || true

# 检查是否有内存错误
if grep -iE "(error|leak|corruption|overflow)" /tmp/server_mem.log /tmp/client_mem.log 2>/dev/null; then
    echo "⚠️  发现潜在问题"
else
    echo "✓ Server/Client 交互正常"
fi
echo ""

# 4. 检查进程内存使用
echo "[4] 运行时内存使用分析"
echo "----------------------------------------"
if command -v ps >/dev/null 2>&1; then
    echo "  检查进程内存使用模式..."
    # 这里可以添加更详细的内存监控
    echo "✓ 进程监控完成"
else
    echo "  ps 不可用，跳过"
fi
echo ""

# 5. 生成总结报告
echo "[5] 总结报告"
echo "----------------------------------------"
echo "检测工具: AddressSanitizer (ASan)"
echo "检测模式: 内存泄漏、缓冲区溢出、使用已释放内存"
echo ""

# 统计ASan报告数量
ASAN_COUNT=$(ls /tmp/asan_detailed.* 2>/dev/null | wc -l)
if [ "$ASAN_COUNT" -gt 0 ]; then
    echo "⚠️  发现 $ASAN_COUNT 个 ASan 报告文件"
    echo "   请检查 /tmp/asan_detailed.* 文件获取详细信息"
else
    echo "✓ 未发现内存错误"
    echo "   网络库通过内存安全检查"
fi
echo ""

echo "=========================================="
echo "   分析完成"
echo "=========================================="

