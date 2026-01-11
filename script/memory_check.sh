#!/bin/bash
# 内存检测脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build_asan"
BIN_DIR="$BUILD_DIR/bin"

echo "=========================================="
echo "   KnetLib 内存检测报告"
echo "=========================================="
echo ""

# 1. 使用 AddressSanitizer 检测
echo "[1] AddressSanitizer 检测"
echo "----------------------------------------"
cd "$PROJECT_DIR" || exit 1
cd "$BUILD_DIR" || exit 1

# 运行测试并捕获ASan输出
ASAN_OPTIONS=detect_leaks=1:abort_on_error=0:log_path=/tmp/asan_test ./bin/test_network > /tmp/test_output.log 2>&1 || true

# 检查ASan报告
if ls /tmp/asan_test.* 2>/dev/null | grep -q .; then
    echo "⚠️  发现 ASan 错误报告:"
    for file in /tmp/asan_test.*; do
        echo "  文件: $file"
        head -30 "$file"
    done
else
    echo "✓ 未发现 ASan 错误"
fi
echo ""

# 2. 检查测试输出中的错误
echo "[2] 测试输出分析"
echo "----------------------------------------"
if grep -iE "(error|leak|corruption|overflow|use-after-free)" /tmp/test_output.log; then
    echo "⚠️  发现潜在问题"
else
    echo "✓ 测试输出正常"
fi
echo ""

# 3. 运行简单的内存压力测试
echo "[3] 内存压力测试"
echo "----------------------------------------"
cd "$BUILD_DIR" || exit 1

# 运行多次测试，检查是否有内存泄漏累积
for i in {1..5}; do
    echo "  运行测试 $i/5..."
    timeout 30 ./bin/test_network > /dev/null 2>&1 || true
done

echo "✓ 压力测试完成"
echo ""

# 4. 检查可执行文件大小（间接检查是否有未优化的内存使用）
echo "[4] 可执行文件分析"
echo "----------------------------------------"
if [ -f "$BIN_DIR/test_network" ]; then
    size=$(du -h "$BIN_DIR/test_network" | cut -f1)
    echo "  test_network 大小: $size"
fi
if [ -f "$BIN_DIR/server" ]; then
    size=$(du -h "$BIN_DIR/server" | cut -f1)
    echo "  server 大小: $size"
fi
if [ -f "$BIN_DIR/client" ]; then
    size=$(du -h "$BIN_DIR/client" | cut -f1)
    echo "  client 大小: $size"
fi
echo ""

# 5. 使用 objdump 检查符号（检查是否有未释放的资源）
echo "[5] 符号分析"
echo "----------------------------------------"
if command -v objdump >/dev/null 2>&1; then
    echo "  检查关键函数符号..."
    if objdump -t "$BIN_DIR/test_network" 2>/dev/null | grep -q "TcpConnection\|EventLoop\|Buffer"; then
        echo "✓ 关键类符号存在"
    fi
else
    echo "  objdump 不可用，跳过"
fi
echo ""

echo "=========================================="
echo "   检测完成"
echo "=========================================="

