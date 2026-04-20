#!/bin/bash

set -euo pipefail

BUILD_DIR="build"
BUILD_APP_DIR="./build/app"
BUILD_LOG_DIR="$BUILD_APP_DIR/log"
LOOP_SCRIPT="$BUILD_APP_DIR/loop.sh"
BUILD_MODEL_SCRIPT="$BUILD_APP_DIR/serving/run_model.sh"
SOURCE_MODEL_SCRIPT="./app/serving/run_model.sh"

run_stop_script() {
	local script_path="$1"
	local description="$2"
	shift 2

	if [[ ! -f "$script_path" ]]; then
		return 1
	fi

	if [[ ! -x "$script_path" ]]; then
		echo "发现 $script_path，但没有可执行权限，正在添加权限并执行..."
		chmod +x "$script_path" || echo "警告：无法为 $script_path 添加执行权限"
	fi

	echo "发现 $script_path，尝试${description}..."
	"$script_path" "$@"
	return 0
}

# 先停止 build/app/loop.sh，由它统一停掉 C++ 服务和模型服务。
if ! run_stop_script "$LOOP_SCRIPT" "停止整套服务" stop; then
	# 如果 loop 脚本不可用，再退回到模型脚本本身，并显式指向 build/app/log 的状态文件。
	if ! PID_STATE_FILE="$BUILD_LOG_DIR/server.pid" \
		PID_LOCK_FILE="$BUILD_LOG_DIR/.server.pid.lock" \
		MODEL_API_LOG_FILE="$BUILD_LOG_DIR/model_api.log" \
		run_stop_script "$BUILD_MODEL_SCRIPT" "停止模型服务" stop; then
		PID_STATE_FILE="$BUILD_LOG_DIR/server.pid" \
		PID_LOCK_FILE="$BUILD_LOG_DIR/.server.pid.lock" \
		MODEL_API_LOG_FILE="$BUILD_LOG_DIR/model_api.log" \
			run_stop_script "$SOURCE_MODEL_SCRIPT" "停止模型服务" stop || true
	fi
fi


# 清理构建目录
if [ -d "$BUILD_DIR" ]; then
	find "$BUILD_DIR" -mindepth 1 -not -name '*.sh' -exec rm -rf {} +
fi

# 创建构建目录
mkdir -p "$BUILD_DIR"

# 运行 cmake 和 make
cmake -S . -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "重新构建完成！"
