#!/usr/bin/env bash
# 测试 GateSvr 基本 gRPC：PushMessage、DisconnectUser（需先有 ZoneSvr，脚本内会先启动 Zone）
# 依赖：grpcurl（可选，未安装时仅启动服务并打印手动测试命令）
# 用法：从项目根目录执行 ./scripts/test_gatesvr.sh

set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

ZONESVR_BIN="${BIN_DIR:-build/backend/zonesvr}/zonesvr"
GATESVR_BIN="${BIN_DIR:-build/backend/gatesvr}/gatesvr"
ZONE_PORT="${ZONESVR_PORT:-9192}"
GATE_GRPC_PORT="${GATESVR_GRPC_PORT:-9191}"
GATE_WS_PORT="${GATESVR_WS_PORT:-9190}"
DATA_DIR="${TMPDIR:-/tmp}/gatesvr_test_$$"
ZONE_CONF="$DATA_DIR/zonesvr.conf"
GATE_CONF="$DATA_DIR/gatesvr.conf"
GRPCURL_OPTS=(-plaintext \
  -import-path backend/common/proto \
  -import-path backend/gatesvr/proto \
  -proto backend/gatesvr/proto/gate.proto)
SVC="swift.gate.GateInternalService"

# 若没有可执行文件则尝试构建
for name in zonesvr gatesvr; do
  BIN="build/backend/$name/$name"
  if [[ ! -x "$BIN" ]]; then
    echo "==> $BIN not found, building..."
    (cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make zonesvr gatesvr) 2>/dev/null || {
      mkdir -p build && (cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make zonesvr gatesvr)
    }
  fi
done
if [[ ! -x "$ZONESVR_BIN" ]]; then
  echo "Error: zonesvr not found at $ZONESVR_BIN" >&2
  exit 1
fi
if [[ ! -x "$GATESVR_BIN" ]]; then
  echo "Error: gatesvr not found at $GATESVR_BIN" >&2
  exit 1
fi

mkdir -p "$DATA_DIR"
cat > "$ZONE_CONF" << EOF
host=127.0.0.1
port=$ZONE_PORT
session_store_type=memory
session_expire_seconds=3600
gate_heartbeat_timeout=60
standalone=1
log_dir=$DATA_DIR
log_level=INFO
EOF
cat > "$GATE_CONF" << EOF
host=127.0.0.1
websocket_port=$GATE_WS_PORT
grpc_port=$GATE_GRPC_PORT
zone_svr_addr=127.0.0.1:$ZONE_PORT
heartbeat_interval_seconds=30
heartbeat_timeout_seconds=90
log_dir=$DATA_DIR
log_level=INFO
EOF

echo "==> Starting ZoneSvr (port=$ZONE_PORT)..."
"$ZONESVR_BIN" "$ZONE_CONF" &
ZONE_PID=$!
sleep 2
if ! kill -0 "$ZONE_PID" 2>/dev/null; then
  echo "Error: ZoneSvr exited early" >&2
  rm -rf "$DATA_DIR"
  exit 1
fi

echo "==> Starting GateSvr (grpc=$GATE_GRPC_PORT)..."
"$GATESVR_BIN" "$GATE_CONF" &
GATE_PID=$!
cleanup() {
  kill -TERM "$GATE_PID" 2>/dev/null || true
  wait "$GATE_PID" 2>/dev/null || true
  kill -TERM "$ZONE_PID" 2>/dev/null || true
  wait "$ZONE_PID" 2>/dev/null || true
  rm -rf "$DATA_DIR"
}
trap cleanup EXIT

sleep 2
if ! kill -0 "$GATE_PID" 2>/dev/null; then
  echo "Error: GateSvr process exited early" >&2
  exit 1
fi

echo "==> Testing GateSvr gRPC (grpcurl)..."
if ! command -v grpcurl >/dev/null 2>&1; then
  echo "grpcurl not installed. Gate is running; manual test commands:"
  echo "  grpcurl ${GRPCURL_OPTS[*]} -d '{\"user_id\":\"u1\",\"cmd\":\"test\",\"payload\":\"\"}' localhost:$GATE_GRPC_PORT $SVC/PushMessage"
  echo "  grpcurl ${GRPCURL_OPTS[*]} -d '{\"user_id\":\"u1\",\"reason\":\"test\"}' localhost:$GATE_GRPC_PORT $SVC/DisconnectUser"
  wait "$GATE_PID"
  exit 0
fi

# 1. PushMessage（用户未连接，预期返回非 0 或 USER_OFFLINE）
PUSH=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u_nobody","cmd":"test","payload":""}' "localhost:$GATE_GRPC_PORT" "$SVC/PushMessage")
echo "PushMessage (u_nobody): $PUSH"
# 未在线时可能 code!=0，这里只要求调用成功、有 code 字段
echo "$PUSH" | grep -q '"code"' || { echo "PushMessage: response should contain code" >&2; exit 1; }

# 2. DisconnectUser（踢人，无连接也返回 OK，幂等）
KICK=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u_any","reason":"test kick"}' "localhost:$GATE_GRPC_PORT" "$SVC/DisconnectUser")
echo "DisconnectUser: $KICK"
CODE=$(echo "$KICK" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
if [[ -n "$CODE" && "$CODE" != "0" ]]; then
  echo "DisconnectUser failed (code=$CODE)" >&2
  exit 1
fi

echo "==> All GateSvr gRPC checks passed."
exit 0
