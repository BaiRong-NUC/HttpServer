#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/../.." && pwd)"
runtimedir="$repo_root/build/app/serving"
pid_file="$runtimedir/model_api.pid"
log_file="$runtimedir/model_api.log"
host="${MODEL_API_HOST:-127.0.0.1}"
port="${MODEL_API_PORT:-8000}"
mode="${1:-fg}"

cd "$repo_root"
mkdir -p "$runtimedir"

python_cmd="python3"
if [[ -x "$repo_root/.venv/bin/python" ]]; then
	python_cmd="$repo_root/.venv/bin/python"
fi

is_running() {
	if [[ ! -f "$pid_file" ]]; then
		return 1
	fi

	local pid
	pid="$(cat "$pid_file")"
	if [[ -z "$pid" ]]; then
		return 1
	fi

	if kill -0 "$pid" >/dev/null 2>&1; then
		return 0
	fi

	rm -f "$pid_file"
	return 1
}

start_background() {
	if is_running; then
		echo "Model service is already running on http://$host:$port (pid $(cat "$pid_file"))"
		return 0
	fi

	nohup "$python_cmd" -m uvicorn app.serving.model_api:app --host "$host" --port "$port" >"$log_file" 2>&1 < /dev/null &
	echo $! > "$pid_file"
	echo "Model service started in background on http://$host:$port"
	echo "PID: $(cat "$pid_file")"
	echo "Log: $log_file"
}

stop_background() {
	if ! is_running; then
		echo "Model service is not running"
		return 0
	fi

	local pid
	pid="$(cat "$pid_file")"
	kill "$pid"

	for _ in $(seq 1 25); do
		if ! kill -0 "$pid" >/dev/null 2>&1; then
			rm -f "$pid_file"
			echo "Model service stopped"
			return 0
		fi
		sleep 0.2
	done

	echo "Model service did not stop within timeout, pid: $pid"
	return 1
}

show_status() {
	if is_running; then
		echo "Model service is running on http://$host:$port (pid $(cat "$pid_file"))"
		echo "Log: $log_file"
	else
		echo "Model service is not running"
	fi
}

show_usage() {
	echo "Usage: $0 [fg|start|stop|restart|status]"
	echo "  fg      Run in foreground with --reload for development (default)"
	echo "  start   Run in background and keep serving after terminal exits"
	echo "  stop    Stop the background service"
	echo "  restart Restart the background service"
	echo "  status  Show background service status"
	echo ""
	echo "Optional env vars: MODEL_API_HOST, MODEL_API_PORT"
}

case "$mode" in
	fg)
		exec "$python_cmd" -m uvicorn app.serving.model_api:app --host "$host" --port "$port" --reload
		;;
	start)
		start_background
		;;
	stop)
		stop_background
		;;
	restart)
		stop_background || true
		start_background
		;;
	status)
		show_status
		;;
	*)
		show_usage
		exit 1
		;;
esac
