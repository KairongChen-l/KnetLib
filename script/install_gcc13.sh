#!/bin/bash
# GCC 13 安装脚本

set -e

echo "========================================="
echo "开始安装 GCC 13"
echo "========================================="

# 更新包列表
echo "1. 更新包列表..."
sudo apt update

# 安装 GCC 13
echo "2. 安装 gcc-13 和 g++-13..."
sudo apt install -y gcc-13 g++-13

# 配置 update-alternatives
echo "3. 配置编译器优先级..."

# 检查是否已存在配置
if ! update-alternatives --list gcc > /dev/null 2>&1; then
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100
fi

if ! update-alternatives --list g++ > /dev/null 2>&1; then
    sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100
fi

# 添加 GCC 13 选项
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 90 2>/dev/null || true
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 90 2>/dev/null || true

# 设置为默认
echo "4. 设置 GCC 13 为默认编译器..."
sudo update-alternatives --set gcc /usr/bin/gcc-13
sudo update-alternatives --set g++ /usr/bin/g++-13

# 验证
echo ""
echo "========================================="
echo "安装完成！验证版本："
echo "========================================="
gcc --version
echo ""
g++ --version
echo ""
echo "========================================="
echo "GCC 13 安装成功！"
echo "========================================="

