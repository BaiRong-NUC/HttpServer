#!/bin/bash

set -e

BUILD_DIR="build"


# 在重构/构建前先尝试停止模型服务（FastAPI）和 loop 脚本
# 先停止模型服务，以避免占用端口或出现文件被占用的问题
MODEL_SCRIPT="./app/serving/run_model.sh"
if [ -f "$MODEL_SCRIPT" ]; then
	if [ -x "$MODEL_SCRIPT" ]; then
		echo "发现 $MODEL_SCRIPT，尝试停止模型服务..."
 		"$MODEL_SCRIPT" stop || echo "警告：停止模型服务返回非零状态"
 	else
 		echo "发现 $MODEL_SCRIPT，但没有可执行权限，正在添加权限并停止..."
 		chmod +x "$MODEL_SCRIPT" || echo "警告：无法为 $MODEL_SCRIPT 添加执行权限"
 		"$MODEL_SCRIPT" stop || echo "警告：停止模型服务返回非零状态"
 	fi
fi

# 如果正在运行 loop 脚本，先停止它
LOOP_SCRIPT="./build/app/loop.sh"
if [ -f "$LOOP_SCRIPT" ]; then
	if [ -x "$LOOP_SCRIPT" ]; then
		echo "发现 $LOOP_SCRIPT，尝试停止..."
		"$LOOP_SCRIPT" stop || echo "警告：停止命令返回非零状态"
	else
		echo "发现 $LOOP_SCRIPT，但没有可执行权限，正在添加权限并停止..."
		chmod +x "$LOOP_SCRIPT" || echo "警告：无法为 $LOOP_SCRIPT 添加执行权限"
		"$LOOP_SCRIPT" stop || echo "警告：停止命令返回非零状态"
	fi
fi


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
