#!/usr/bin/env bash
# 测试 ZoneSvr 基本 gRPC：GateRegister、GateHeartbeat、UserOnline、GetUserStatus、UserOffline
# 依赖：grpcurl（可选，未安装时仅启动服务并打印手动测试命令）
# 用法：从项目根目录执行 ./scripts/test_zonesvr.sh

set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

ZONESVR_BIN="${BIN_DIR:-build/backend/zonesvr}/zonesvr"
PORT="${ZONESVR_PORT:-9092}"
DATA_DIR="${TMPDIR:-/tmp}/zonesvr_test_$$"
CONF="$DATA_DIR/zonesvr.conf"
GRPCURL_OPTS=(-plaintext \
  -import-path backend/common/proto \
  -import-path backend/zonesvr/proto \
  -proto backend/zonesvr/proto/zone.proto)
SVC="swift.zone.ZoneService"

# 若没有可执行文件则尝试构建
if [[ ! -x "$ZONESVR_BIN" ]]; then
  echo "==> $ZONESVR_BIN not found, building..."
  (cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make zonesvr) 2>/dev/null || {
    mkdir -p build && (cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make zonesvr)
  }
fi
if [[ ! -x "$ZONESVR_BIN" ]]; then
  echo "Error: zonesvr binary not found at $ZONESVR_BIN" >&2
  exit 1
fi

mkdir -p "$DATA_DIR"
cat > "$CONF" << EOF
host=127.0.0.1
port=$PORT
session_store_type=memory
session_expire_seconds=3600
gate_heartbeat_timeout=60
standalone=1
log_dir=$DATA_DIR
log_level=INFO
EOF

echo "==> Starting ZoneSvr (port=$PORT, config=$CONF)..."
"$ZONESVR_BIN" "$CONF" &
ZONESVR_PID=$!
cleanup() {
  kill -TERM "$ZONESVR_PID" 2>/dev/null || true
  wait "$ZONESVR_PID" 2>/dev/null || true
  rm -rf "$DATA_DIR"
}
trap cleanup EXIT

sleep 2
if ! kill -0 "$ZONESVR_PID" 2>/dev/null; then
  echo "Error: ZoneSvr process exited early" >&2
  exit 1
fi
# 等待端口就绪
for i in 1 2 3 4 5; do
  if grpcurl -plaintext "127.0.0.1:$PORT" list >/dev/null 2>&1; then break; fi
  sleep 1
done

echo "==> Testing ZoneSvr gRPC (grpcurl)..."
if ! command -v grpcurl >/dev/null 2>&1; then
  echo "grpcurl not installed. Server is running; manual test commands:"
  echo "  grpcurl ${GRPCURL_OPTS[*]} -d '{\"gate_id\":\"gate_test\",\"address\":\"localhost:9091\",\"current_connections\":0}' 127.0.0.1:$PORT $SVC/GateRegister"
  echo "  grpcurl ${GRPCURL_OPTS[*]} -d '{\"gate_id\":\"gate_test\",\"current_connections\":0}' 127.0.0.1:$PORT $SVC/GateHeartbeat"
  echo "  grpcurl ${GRPCURL_OPTS[*]} -d '{\"user_id\":\"u1\",\"gate_id\":\"gate_test\",\"device_type\":\"web\",\"device_id\":\"d1\"}' 127.0.0.1:$PORT $SVC/UserOnline"
  echo "  grpcurl ${GRPCURL_OPTS[*]} -d '{\"user_ids\":[\"u1\"]}' 127.0.0.1:$PORT $SVC/GetUserStatus"
  echo "  grpcurl ${GRPCURL_OPTS[*]} -d '{\"user_id\":\"u1\",\"gate_id\":\"gate_test\"}' 127.0.0.1:$PORT $SVC/UserOffline"
  wait "$ZONESVR_PID"
  exit 0
fi

TARGET="127.0.0.1:$PORT"
# 1. GateRegister
REG=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"gate_id":"gate_test","address":"localhost:9091","current_connections":0}' "$TARGET" "$SVC/GateRegister")
echo "GateRegister: $REG"
CODE=$(echo "$REG" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
if [[ -n "$CODE" && "$CODE" != "0" ]]; then
  echo "GateRegister failed (code=$CODE)" >&2
  exit 1
fi

# 2. GateHeartbeat
HB=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"gate_id":"gate_test","current_connections":0}' "$TARGET" "$SVC/GateHeartbeat")
echo "GateHeartbeat: $HB"
CODE=$(echo "$HB" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
if [[ -n "$CODE" && "$CODE" != "0" ]]; then
  echo "GateHeartbeat failed (code=$CODE)" >&2
  exit 1
fi

# 3. UserOnline
ON=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u1","gate_id":"gate_test","device_type":"web","device_id":"d1"}' "$TARGET" "$SVC/UserOnline")
echo "UserOnline: $ON"
CODE=$(echo "$ON" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
if [[ -n "$CODE" && "$CODE" != "0" ]]; then
  echo "UserOnline failed (code=$CODE)" >&2
  exit 1
fi

# 4. GetUserStatus
STATUS=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_ids":["u1"]}' "$TARGET" "$SVC/GetUserStatus")
echo "GetUserStatus: $STATUS"
CODE=$(echo "$STATUS" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
if [[ -n "$CODE" && "$CODE" != "0" ]]; then
  echo "GetUserStatus failed (code=$CODE)" >&2
  exit 1
fi
echo "$STATUS" | grep -qE '"online"\s*:\s*true' || { echo "GetUserStatus: u1 should be online" >&2; exit 1; }

# 5. UserOffline
OFF=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u1","gate_id":"gate_test"}' "$TARGET" "$SVC/UserOffline")
echo "UserOffline: $OFF"
CODE=$(echo "$OFF" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
if [[ -n "$CODE" && "$CODE" != "0" ]]; then
  echo "UserOffline failed (code=$CODE)" >&2
  exit 1
fi

# 6. GetUserStatus again (u1 should be offline)
STATUS2=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_ids":["u1"]}' "$TARGET" "$SVC/GetUserStatus")
echo "GetUserStatus (after offline): $STATUS2"
# 离线后不应再为 true（proto3 JSON 可能省略 false）
echo "$STATUS2" | grep -qE '"online"\s*:\s*true' && { echo "GetUserStatus: u1 should be offline" >&2; exit 1; }

echo "==> All ZoneSvr gRPC checks passed."
exit 0
