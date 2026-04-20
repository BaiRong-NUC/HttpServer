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
SUP_PID_FILE="$PID_DIR/server_supervisor.pid"
SRV_PID_FILE="$PID_DIR/server.pid"
LOG_FILE="$PID_DIR/server.log"
STOP_SCRIPT="$PID_DIR/stop_app.sh"
SUP_SCRIPT="$PID_DIR/server_supervisor.sh"
MODEL_SCRIPT="$(detect_model_script)"

usage() {
	echo "Usage: $0 [start|stop|restart|status]"
	exit 1
}

is_supervisor_running() {
	if [[ ! -f "$SUP_PID_FILE" ]]; then
		return 1
	fi

	local pid
	pid="$(cat "$SUP_PID_FILE")"
	if [[ -z "$pid" ]]; then
		return 1
	fi

	kill -0 "$pid" >/dev/null 2>&1
}

create_stop_script() {
	cat > "$STOP_SCRIPT" <<SH
#!/bin/bash
set -euo pipefail
SUP_PID_FILE="$SUP_PID_FILE"
SRV_PID_FILE="$SRV_PID_FILE"
MODEL_SCRIPT="$MODEL_SCRIPT"

if [[ -n "\$MODEL_SCRIPT" && -x "\$MODEL_SCRIPT" ]]; then
	"\$MODEL_SCRIPT" stop || true
fi

if [[ -f "\$SUP_PID_FILE" ]]; then
	SUPPID="\$(cat "\$SUP_PID_FILE")"
	kill "\$SUPPID" 2>/dev/null || true
	sleep 1
	if kill -0 "\$SUPPID" 2>/dev/null; then
		kill -9 "\$SUPPID" 2>/dev/null || true
	fi
fi

if [[ -f "\$SRV_PID_FILE" ]]; then
	PID="\$(cat "\$SRV_PID_FILE")"
	kill "\$PID" 2>/dev/null || true
fi

rm -f "\$SUP_PID_FILE" "\$SRV_PID_FILE"
echo "Stopped."
SH
	chmod +x "$STOP_SCRIPT"
}

create_supervisor_script() {
	cat > "$SUP_SCRIPT" <<SH
#!/bin/bash
set -euo pipefail
APP_DIR="$APP_DIR"
SRV_PID_FILE="$SRV_PID_FILE"
LOG_FILE="$LOG_FILE"

cd "$APP_DIR"
while true; do
	./server >> "$LOG_FILE" 2>&1 &
	CHILD=\$!
	echo \$CHILD > "$SRV_PID_FILE"
	wait \$CHILD
	echo "\$(date '+%F %T') server exited with \$?, restarting in 1s" >> "$LOG_FILE"
	sleep 1
done
SH
	chmod +x "$SUP_SCRIPT"
}

start_server_supervisor() {
	if is_supervisor_running; then
		echo "Supervisor already running (PID $(cat "$SUP_PID_FILE"))."
		return 0
	fi

	create_supervisor_script
	nohup "$SUP_SCRIPT" >/dev/null 2>&1 &
	echo $! > "$SUP_PID_FILE"
	echo "Started server supervisor (PID $(cat "$SUP_PID_FILE")). Logs: $LOG_FILE"
}

stop_server_supervisor() {
	if [[ -f "$SUP_PID_FILE" ]]; then
		local sup_pid
		sup_pid="$(cat "$SUP_PID_FILE")"
		kill "$sup_pid" 2>/dev/null || true
		sleep 1
		if kill -0 "$sup_pid" 2>/dev/null; then
			kill -9 "$sup_pid" 2>/dev/null || true
		fi
	fi

	if [[ -f "$SRV_PID_FILE" ]]; then
		local server_pid
		server_pid="$(cat "$SRV_PID_FILE")"
		kill "$server_pid" 2>/dev/null || true
	fi

	rm -f "$SUP_PID_FILE" "$SRV_PID_FILE"
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
		echo "Server supervisor is running (PID $(cat "$SUP_PID_FILE"))."
	else
		echo "Server supervisor is not running."
	fi

	if [[ -n "$MODEL_SCRIPT" && -x "$MODEL_SCRIPT" ]]; then
		"$MODEL_SCRIPT" status
	else
		echo "Model service script not found."
	fi
}

do_start() {
	create_stop_script
	start_server_supervisor
	start_model_service
}

do_stop() {
	stop_model_service
	stop_server_supervisor
	echo "Stopped."
}

case "${1:-start}" in
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