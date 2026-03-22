#!/bin/bash
# Supervisor wrapper: 启动可自动重启的后台进程并生成停止脚本
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# 自动检测 server 可执行文件所在目录：优先使用与脚本同目录，其次上一级，再次 look for app 子目录
if [ -x "$SCRIPT_DIR/server" ]; then
	APP_DIR="$SCRIPT_DIR"
elif [ -x "$SCRIPT_DIR/../server" ]; then
	APP_DIR="$SCRIPT_DIR/.."
elif [ -d "$SCRIPT_DIR/app" ]; then
	APP_DIR="$SCRIPT_DIR/app"
else
	APP_DIR="$SCRIPT_DIR"
fi
PID_DIR="$SCRIPT_DIR"
SUP_PID_FILE="$PID_DIR/server_supervisor.pid"
SRV_PID_FILE="$PID_DIR/server.pid"
LOG_FILE="$PID_DIR/server.log"
STOP_SCRIPT="$PID_DIR/stop_app.sh"
SUP_SCRIPT="$PID_DIR/server_supervisor.sh"

usage() {
	echo "Usage: $0 [start|stop|restart]"
	exit 1
}

create_stop_script() {
	cat > "$STOP_SCRIPT" <<'SH'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
SUP_PID_FILE="$DIR/server_supervisor.pid"
SRV_PID_FILE="$DIR/server.pid"
if [ -f "$SUP_PID_FILE" ]; then
	SUPPID=$(cat "$SUP_PID_FILE")
	kill "$SUPPID" 2>/dev/null || true
	sleep 1
	if kill -0 "$SUPPID" 2>/dev/null; then
		kill -9 "$SUPPID" 2>/dev/null || true
	fi
fi
if [ -f "$SRV_PID_FILE" ]; then
	PID=$(cat "$SRV_PID_FILE")
	kill "$PID" 2>/dev/null || true
fi
rm -f "$SUP_PID_FILE" "$SRV_PID_FILE"
echo "Stopped."
SH
	chmod +x "$STOP_SCRIPT"
}

create_supervisor_script() {
		cat > "$SUP_SCRIPT" <<'SH'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
# detect server location relative to supervisor script
if [ -x "$DIR/server" ]; then
	APP_DIR="$DIR"
elif [ -x "$DIR/../server" ]; then
	APP_DIR="$DIR/.."
elif [ -d "$DIR/app" ]; then
	APP_DIR="$DIR/app"
else
	APP_DIR="$DIR"
fi
SRV_PID_FILE="$DIR/server.pid"
LOG_FILE="$DIR/server.log"
cd "$APP_DIR"
while true; do
	./server >> "$LOG_FILE" 2>&1 &
	CHILD=$!
	echo $CHILD > "$SRV_PID_FILE"
	wait $CHILD
	echo "$(date '+%F %T') server exited with $? , restarting in 1s" >> "$LOG_FILE"
	sleep 1
done
SH
	chmod +x "$SUP_SCRIPT"
}

do_start() {
	if [ -f "$SUP_PID_FILE" ] && kill -0 $(cat "$SUP_PID_FILE") 2>/dev/null; then
		echo "Supervisor already running (PID $(cat $SUP_PID_FILE))."
		exit 0
	fi
	create_stop_script
	create_supervisor_script
	nohup "$SUP_SCRIPT" >/dev/null 2>&1 &
	echo $! > "$SUP_PID_FILE"
	echo "Started supervisor (PID $(cat $SUP_PID_FILE)). Logs: $LOG_FILE"
}

do_stop() {
	if [ -x "$STOP_SCRIPT" ]; then
		"$STOP_SCRIPT"
	else
		echo "No stop script found. Attempting to stop directly."
		if [ -f "$SUP_PID_FILE" ]; then
			kill $(cat "$SUP_PID_FILE") 2>/dev/null || true
		fi
		if [ -f "$SRV_PID_FILE" ]; then
			kill $(cat "$SRV_PID_FILE") 2>/dev/null || true
		fi
		rm -f "$SUP_PID_FILE" "$SRV_PID_FILE"
	fi
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
	*)
		usage
		;;
esac

exit 0