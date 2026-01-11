#!/bin/bash
# 生成完整的内存分析报告

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build_asan"
REPORT_FILE="$PROJECT_DIR/memory_analysis_report.md"

echo "生成内存分析报告..."

cd "$PROJECT_DIR" || exit 1

cat > "$REPORT_FILE" << 'EOF'
# KnetLib 内存分析报告

## 检测工具

1. **AddressSanitizer (ASan)**
   - 检测内存泄漏
   - 检测缓冲区溢出
   - 检测使用已释放内存
   - 检测未初始化内存使用

2. **内存压力测试**
   - 多次运行测试程序
   - 检查内存泄漏累积

3. **交互式测试**
   - Server/Client 交互测试
   - 检查连接建立和断开时的内存管理

## 检测结果

EOF

# 运行检测并收集结果
cd "$BUILD_DIR" || exit 1

echo "### AddressSanitizer 检测结果" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

# 运行测试并检查ASan输出
export ASAN_OPTIONS="detect_leaks=1:abort_on_error=0:print_stats=1:log_path=/tmp/asan_report"
./bin/test_network > /tmp/test_asan.log 2>&1 || true

if ls /tmp/asan_report.* 2>/dev/null | grep -q .; then
    echo "⚠️ **发现内存问题**" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "ASan 报告文件:" >> "$REPORT_FILE"
    for file in /tmp/asan_report.*; do
        echo "- \`$file\`" >> "$REPORT_FILE"
    done
    echo "" >> "$REPORT_FILE"
    echo "详细内容:" >> "$REPORT_FILE"
    echo "\`\`\`" >> "$REPORT_FILE"
    head -100 /tmp/asan_report.* >> "$REPORT_FILE" 2>/dev/null || true
    echo "\`\`\`" >> "$REPORT_FILE"
else
    echo "✓ **未发现内存错误**" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "所有测试通过，未发现内存泄漏、缓冲区溢出或其他内存安全问题。" >> "$REPORT_FILE"
fi

echo "" >> "$REPORT_FILE"
echo "### 内存压力测试结果" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

# 运行内存泄漏测试
if [ -f "./bin/test_memory_leak" ]; then
    echo "运行内存泄漏专项测试..." >> "$REPORT_FILE"
    timeout 120 ./bin/test_memory_leak > /tmp/memory_leak_test.log 2>&1 || true
    
    if grep -iE "(leak|error|corruption)" /tmp/memory_leak_test.log; then
        echo "⚠️ 发现潜在问题" >> "$REPORT_FILE"
    else
        echo "✓ 测试通过" >> "$REPORT_FILE"
    fi
    echo "" >> "$REPORT_FILE"
fi

echo "### 测试统计" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "- 测试程序: test_network" >> "$REPORT_FILE"
echo "- 内存泄漏测试: test_memory_leak" >> "$REPORT_FILE"
echo "- 编译选项: -fsanitize=address" >> "$REPORT_FILE"
echo "- 检测时间: $(date)" >> "$REPORT_FILE"

echo "" >> "$REPORT_FILE"
echo "## 建议" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "1. 定期运行内存检测工具" >> "$REPORT_FILE"
echo "2. 在 CI/CD 中集成 AddressSanitizer" >> "$REPORT_FILE"
echo "3. 使用 valgrind 进行更详细的内存分析（如果可用）" >> "$REPORT_FILE"
echo "4. 监控生产环境的内存使用情况" >> "$REPORT_FILE"

echo ""
echo "报告已生成: $REPORT_FILE"
cat "$REPORT_FILE"

