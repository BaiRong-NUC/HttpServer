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
PID_FILE="$PID_DIR/server.pid"
LOG_FILE="$PID_DIR/server.log"
SELF_SCRIPT="$SCRIPT_DIR/$(basename "$0")"
MODEL_SCRIPT="$(detect_model_script)"

usage() {
	echo "Usage: $0 [start|stop|restart|status]"
	exit 1
}

is_supervisor_running() {
	local pid
	pid="$(get_supervisor_pid)"
	if [[ -z "$pid" ]]; then
		return 1
	fi

		kill -0 "$pid" >/dev/null 2>&1
}

get_pid_value() {
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

write_pid_state() {
	local supervisor_pid="$1"
	local server_pid="$2"

	cat > "$PID_FILE" <<EOF
supervisor_pid=$supervisor_pid
server_pid=$server_pid
EOF
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

run_server_supervisor() {
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

	rm -f "$PID_FILE"
}

start_model_service() {
	if [[ -n "$MODEL_SCRIPT" && -x "$MODEL_SCRIPT" ]]; then
		"$MODEL_SCRIPT" start
	else
		echo "Model service script not found, skipped."
	fi
}

stop_model_service() {
	if [[ -n "$MODEL_SCRIPT" && -x "$MODEL_SCRIPT" ]]; then
		"$MODEL_SCRIPT" stop || true
	fi
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
		"$MODEL_SCRIPT" status
	else
		echo "Model service script not found."
	fi
}

do_start() {
	start_server_supervisor
	start_model_service
}

do_stop() {
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