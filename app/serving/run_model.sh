#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
app_root="$(cd "$script_dir/.." && pwd)"
repo_root="$(cd "$app_root/.." && pwd)"
project_root="$repo_root"

if [[ ! -f "$project_root/CMakeLists.txt" && -f "$project_root/../CMakeLists.txt" ]]; then
	project_root="$(cd "$project_root/.." && pwd)"
fi

if [[ -f "$repo_root/CMakeCache.txt" || -d "$repo_root/CMakeFiles" ]]; then
	work_root="$repo_root"
	runtimedir="$app_root/serving"
else
	work_root="$project_root"
	runtimedir="$project_root/build/app/serving"
fi

log_dir="${APP_LOG_DIR:-$app_root/log}"
pid_state_file="${PID_STATE_FILE:-$log_dir/server.pid}"
pid_lock_file="${PID_LOCK_FILE:-$log_dir/.server.pid.lock}"
log_file="${MODEL_API_LOG_FILE:-$log_dir/model_api.log}"
legacy_pid_file="$runtimedir/model_api.pid"
host="${MODEL_API_HOST:-127.0.0.1}"
port="${MODEL_API_PORT:-8000}"
mode="${1:-fg}"

cd "$work_root"
mkdir -p "$runtimedir" "$log_dir"

python_cmd=("python3")
python_source="system python3"

resolve_python_cmd() {
	local conda_cmd=""
	local conda_env_prefix=""
	local env_name=""

	if [[ -n "${CONDA_EXE:-}" && -x "${CONDA_EXE}" ]]; then
		conda_cmd="$CONDA_EXE"
	elif command -v conda >/dev/null 2>&1; then
		conda_cmd="$(command -v conda)"
	elif [[ -x "$HOME/miniconda3/bin/conda" ]]; then
		conda_cmd="$HOME/miniconda3/bin/conda"
	fi

	if [[ -n "$conda_cmd" ]]; then
		for env_name in web sklearn; do
			conda_env_prefix="$("$conda_cmd" env list 2>/dev/null | awk -v target="$env_name" '$1 == target { print $NF; exit }')"
			if [[ -n "$conda_env_prefix" && -x "$conda_env_prefix/bin/python" ]]; then
				python_cmd=("$conda_env_prefix/bin/python")
				python_source="conda env $env_name"
				return
			fi
		done
	fi

	if [[ -x "$project_root/.venv/bin/python" ]]; then
		python_cmd=("$project_root/.venv/bin/python")
		python_source="project .venv"
	fi
}

resolve_python_cmd

with_pid_lock() {
	if command -v flock >/dev/null 2>&1; then
		local lock_fd
		exec {lock_fd}> "$pid_lock_file"
		flock "$lock_fd"
		"$@"
		local status=$?
		flock -u "$lock_fd"
		exec {lock_fd}>&-
		return $status
	fi

	"$@"
}

read_pid_value_unlocked() {
	local key="$1"

	if [[ -f "$pid_state_file" ]]; then
		while IFS='=' read -r current_key current_value; do
			if [[ "$current_key" == "$key" ]]; then
				printf '%s\n' "$current_value"
				return 0
			fi
		done < "$pid_state_file"
	fi

	return 1
}

write_pid_state_impl() {
	local supervisor_pid="$1"
	local server_pid="$2"
	local model_pid="$3"
	local temp_file

	temp_file="$(mktemp "$log_dir/server.pid.tmp.XXXXXX")"
	cat > "$temp_file" <<EOF
supervisor_pid=$supervisor_pid
server_pid=$server_pid
model_pid=$model_pid
EOF
	mv "$temp_file" "$pid_state_file"
}

get_model_pid() {
	local pid
	pid="$(read_pid_value_unlocked model_pid || true)"
	if [[ -n "$pid" ]]; then
		printf '%s\n' "$pid"
		return 0
	fi

	return 1
}

set_model_pid() {
	with_pid_lock write_pid_state_impl \
		"$(read_pid_value_unlocked supervisor_pid || true)" \
		"$(read_pid_value_unlocked server_pid || true)" \
		"$1"
	rm -f "$legacy_pid_file"
}

is_running() {
	local pid
	pid="$(get_model_pid || true)"
	if [[ -z "$pid" ]]; then
		return 1
	fi

	if kill -0 "$pid" >/dev/null 2>&1; then
		return 0
	fi

	set_model_pid ""
	return 1
}

start_background() {
	if is_running; then
		echo "Model service is already running on http://$host:$port (pid $(get_model_pid))"
		return 0
	fi

	nohup "${python_cmd[@]}" -m uvicorn app.serving.model_api:app --host "$host" --port "$port" >"$log_file" 2>&1 < /dev/null &
	set_model_pid "$!"
	echo "Model service started in background on http://$host:$port"
	echo "Python: $python_source"
	echo "PID: $(get_model_pid)"
	echo "Log: $log_file"
}

stop_background() {
	if ! is_running; then
		echo "Model service is not running"
		return 0
	fi

	local pid
	pid="$(get_model_pid)"
	kill "$pid"

	for _ in $(seq 1 25); do
		if ! kill -0 "$pid" >/dev/null 2>&1; then
			set_model_pid ""
			echo "Model service stopped"
			return 0
		fi
		sleep 0.2
	done

	echo "Model service did not stop within timeout, pid: $pid"
	return 1
}

show_status() {
	echo "Python: $python_source"
	if is_running; then
		echo "Model service is running on http://$host:$port (pid $(get_model_pid))"
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
		echo "Python: $python_source"
		exec "${python_cmd[@]}" -m uvicorn app.serving.model_api:app --host "$host" --port "$port" --reload
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
