#!/bin/bash

set -e

BUILD_DIR="build"

# 清理构建目录
if [ -d "$BUILD_DIR" ]; then
	find "$BUILD_DIR" -mindepth 1 -not -name '*.sh' -exec rm -rf {} +
fi

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 运行 cmake 和 make
cmake ..
make -j$(nproc)

cd ..
echo "重新构建完成！"
