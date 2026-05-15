#!/bin/bash
# Supervisor wrapper: 启动 C++ server，并联动管理 Python model service
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

detect_app_dir() {
	if [[ -x "$SCRIPT_DIR/server" ]]; then
		echo "$SCRIPT_DIR"
	elif [[ -x "$SCRIPT_DIR/../server" ]]; then
		echo "$(cd "$SCRIPT_DIR/.." && pwd)"
	elif [[ -x "$SCRIPT_DIR/../build/app/server" ]]; then
		echo "$(cd "$SCRIPT_DIR/../build/app" && pwd)"
	elif [[ -d "$SCRIPT_DIR/app" ]]; then
		echo "$SCRIPT_DIR/app"
	else
		echo "$SCRIPT_DIR"
	fi
}

detect_model_script() {
	if [[ -x "$SCRIPT_DIR/serving/run_model.sh" ]]; then
		echo "$SCRIPT_DIR/serving/run_model.sh"
	elif [[ -x "$APP_DIR/serving/run_model.sh" ]]; then
		echo "$APP_DIR/serving/run_model.sh"
	elif [[ -x "$SCRIPT_DIR/../app/serving/run_model.sh" ]]; then
		echo "$(cd "$SCRIPT_DIR/../app/serving" && pwd)/run_model.sh"
	fi
}

APP_DIR="$(detect_app_dir)"
PID_DIR="$SCRIPT_DIR"
LOG_DIR="$APP_DIR/log"
PID_FILE="$LOG_DIR/server.pid"
PID_LOCK_FILE="$LOG_DIR/.server.pid.lock"
LOG_FILE="$LOG_DIR/server.log"
MODEL_LOG_FILE="$LOG_DIR/model_api.log"
SELF_SCRIPT="$SCRIPT_DIR/$(basename "$0")"
MODEL_SCRIPT="$(detect_model_script)"
MOSAIC_PY_PATH="${MOSAIC_PY_PATH:-/home/bairong/C++/MosaicRestored/app/serving/mosaic.py}"
MOSAIC_LOG_FILE="$LOG_DIR/mosaic_restored.log"

detect_mosaic_python() {
	if [[ -n "${MOSAIC_PYTHON:-}" ]]; then
		echo "$MOSAIC_PYTHON"
		return 0
	fi

	if command -v conda >/dev/null 2>&1; then
		local sklearn_env_path
		sklearn_env_path="$(conda env list | awk '$1 == "sklearn" { print $NF; exit }')"
		if [[ -n "$sklearn_env_path" && -x "$sklearn_env_path/bin/python" ]]; then
			echo "$sklearn_env_path/bin/python"
			return 0
		fi
	fi

	if [[ -n "${PYTHON:-}" ]]; then
		echo "$PYTHON"
		return 0
	fi

	echo "python3"
}

MOSAIC_PYTHON="$(detect_mosaic_python)"

usage() {
	echo "Usage: $0 [start|stop|restart|status]"
	exit 1
}

ensure_log_dir() {
	mkdir -p "$LOG_DIR"
}

with_pid_lock() {
	ensure_log_dir
	if command -v flock >/dev/null 2>&1; then
		local lock_fd
		exec {lock_fd}> "$PID_LOCK_FILE"
		flock "$lock_fd"
		"$@"
		local status=$?
		flock -u "$lock_fd"
		exec {lock_fd}>&-
		return $status
	fi

	"$@"
}

is_supervisor_running() {
	local pid
	pid="$(get_supervisor_pid || true)"
	if [[ -z "$pid" ]]; then
		return 1
	fi

	kill -0 "$pid" >/dev/null 2>&1
}

read_pid_value_unlocked() {
	local key="$1"

	if [[ -f "$PID_FILE" ]]; then
		while IFS='=' read -r current_key current_value; do
			if [[ "$current_key" == "$key" ]]; then
				printf '%s\n' "$current_value"
				return 0
			fi
		done < "$PID_FILE"
	fi

	return 1
}

get_pid_value() {
	read_pid_value_unlocked "$1"
}

write_pid_state_impl() {
	local supervisor_pid="$1"
	local server_pid="$2"
	local model_pid="${3:-$(read_pid_value_unlocked model_pid || true)}"
	local mosaic_pid="${4:-$(read_pid_value_unlocked mosaic_pid || true)}"
	local temp_file

	ensure_log_dir
	temp_file="$(mktemp "$LOG_DIR/server.pid.tmp.XXXXXX")"
	cat > "$temp_file" <<EOF
supervisor_pid=$supervisor_pid
server_pid=$server_pid
model_pid=$model_pid
mosaic_pid=$mosaic_pid
EOF
	mv "$temp_file" "$PID_FILE"
}

write_pid_state() {
	with_pid_lock write_pid_state_impl "$@"
}

set_model_pid() {
	with_pid_lock write_pid_state_impl \
		"$(read_pid_value_unlocked supervisor_pid || true)" \
		"$(read_pid_value_unlocked server_pid || true)" \
		"$1"
}

set_mosaic_pid() {
	with_pid_lock write_pid_state_impl \
		"$(read_pid_value_unlocked supervisor_pid || true)" \
		"$(read_pid_value_unlocked server_pid || true)" \
		"$(read_pid_value_unlocked model_pid || true)" \
		"$1"
}

get_supervisor_pid() {
	local pid
	pid="$(get_pid_value supervisor_pid || true)"
	if [[ -n "$pid" ]]; then
		printf '%s\n' "$pid"
		return 0
	fi

	return 1
}

get_server_pid() {
	local pid
	pid="$(get_pid_value server_pid || true)"
	if [[ -n "$pid" ]]; then
		printf '%s\n' "$pid"
		return 0
	fi

	if [[ -f "$PID_FILE" ]]; then
		pid="$(tr -d '\n' < "$PID_FILE")"
		if [[ "$pid" =~ ^[0-9]+$ ]]; then
			printf '%s\n' "$pid"
			return 0
		fi
	fi

	return 1
}

get_mosaic_pid() {
	local pid
	pid="$(get_pid_value mosaic_pid || true)"
	if [[ -n "$pid" ]]; then
		printf '%s\n' "$pid"
		return 0
	fi

	return 1
}

run_server_supervisor() {
	ensure_log_dir
	cd "$APP_DIR"
	write_pid_state "$$" ""
	while true; do
		./server >> "$LOG_FILE" 2>&1 &
		CHILD=$!
		write_pid_state "$$" "$CHILD"
		wait $CHILD
		write_pid_state "$$" ""
		echo "$(date '+%F %T') server exited with $?, restarting in 1s" >> "$LOG_FILE"
		sleep 1
	done
}

start_server_supervisor() {
	if is_supervisor_running; then
		echo "Supervisor already running (PID $(get_supervisor_pid))."
		return 0
	fi

	ensure_log_dir
	nohup "$SELF_SCRIPT" __supervise >/dev/null 2>&1 &
	write_pid_state "$!" ""
	echo "Started server supervisor (PID $(get_supervisor_pid)). Logs: $LOG_FILE"
}

stop_server_supervisor() {
	local sup_pid
	sup_pid="$(get_supervisor_pid || true)"
	if [[ -n "$sup_pid" ]]; then
		kill "$sup_pid" 2>/dev/null || true
		sleep 1
		if kill -0 "$sup_pid" 2>/dev/null; then
			kill -9 "$sup_pid" 2>/dev/null || true
		fi
	fi

	local server_pid
	server_pid="$(get_server_pid || true)"
	if [[ -n "$server_pid" ]]; then
		kill "$server_pid" 2>/dev/null || true
	fi

	rm -f "$PID_FILE" "$PID_LOCK_FILE"
}

start_model_service() {
	if [[ -n "$MODEL_SCRIPT" && -x "$MODEL_SCRIPT" ]]; then
		PID_STATE_FILE="$PID_FILE" PID_LOCK_FILE="$PID_LOCK_FILE" MODEL_API_LOG_FILE="$MODEL_LOG_FILE" \
			"$MODEL_SCRIPT" start
	else
		echo "Model service script not found, skipped."
	fi
}

start_mosaic_service() {
	if [[ ! -f "$MOSAIC_PY_PATH" ]]; then
		echo "MosaicRestored mosaic.py not found, skipped."
		return 0
	fi

	local pid
	pid="$(get_mosaic_pid || true)"
	if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then
		echo "MosaicRestored service already running (PID $pid)."
		return 0
	fi

	ensure_log_dir
	nohup "$MOSAIC_PYTHON" "$MOSAIC_PY_PATH" >> "$MOSAIC_LOG_FILE" 2>&1 &
	pid=$!
	set_mosaic_pid "$pid"
	echo "Started MosaicRestored service (PID $pid). Logs: $MOSAIC_LOG_FILE"
}

stop_model_service() {
	if [[ -n "$MODEL_SCRIPT" && -x "$MODEL_SCRIPT" ]]; then
		PID_STATE_FILE="$PID_FILE" PID_LOCK_FILE="$PID_LOCK_FILE" MODEL_API_LOG_FILE="$MODEL_LOG_FILE" \
			"$MODEL_SCRIPT" stop || true
	fi
}

stop_mosaic_service() {
	local pid
	pid="$(get_mosaic_pid || true)"
	if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then
		kill "$pid" 2>/dev/null || true
		sleep 1
		if kill -0 "$pid" 2>/dev/null; then
			kill -9 "$pid" 2>/dev/null || true
		fi
		echo "Stopped MosaicRestored service (PID $pid)."
	fi
	set_mosaic_pid ""
}

show_status() {
	if is_supervisor_running; then
		echo "Server supervisor is running (PID $(get_supervisor_pid))."
	else
		echo "Server supervisor is not running."
	fi

	local server_pid
	server_pid="$(get_server_pid || true)"
	if [[ -n "$server_pid" ]] && kill -0 "$server_pid" >/dev/null 2>&1; then
		echo "Server process is running (PID $server_pid)."
	elif [[ -n "$server_pid" ]]; then
		echo "Server process PID recorded but not running (PID $server_pid)."
	fi

	if [[ -n "$MODEL_SCRIPT" && -x "$MODEL_SCRIPT" ]]; then
		PID_STATE_FILE="$PID_FILE" PID_LOCK_FILE="$PID_LOCK_FILE" MODEL_API_LOG_FILE="$MODEL_LOG_FILE" \
			"$MODEL_SCRIPT" status
	else
		echo "Model service script not found."
	fi

	local mosaic_pid
	mosaic_pid="$(get_mosaic_pid || true)"
	if [[ -n "$mosaic_pid" ]] && kill -0 "$mosaic_pid" >/dev/null 2>&1; then
		echo "MosaicRestored service is running (PID $mosaic_pid)."
	elif [[ -n "$mosaic_pid" ]]; then
		echo "MosaicRestored service PID recorded but not running (PID $mosaic_pid)."
	else
		echo "MosaicRestored service not running."
	fi
}

do_start() {
	start_server_supervisor
	start_model_service
	start_mosaic_service
}

do_stop() {
	stop_mosaic_service
	stop_model_service
	stop_server_supervisor
	echo "Stopped."
}

case "${1:-start}" in
	__supervise)
		run_server_supervisor
		;;
	start)
		do_start
		;;
	stop)
		do_stop
		;;
	restart)
		do_stop
		sleep 1
		do_start
		;;
	status)
		show_status
		;;
	*)
		usage
		;;
esac

exit 0