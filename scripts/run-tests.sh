#!/usr/bin/env bash
# 单测入口：配置加载、AuthSvr、ZoneSvr、GateSvr。由 Makefile 的 make test 调用。
set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# ---------- 1. 配置加载测试 ----------
run_test_config_load() {
  echo "===== test: config load ====="
  local BUILD="${BUILD:-./build/backend}"
  local RUN_TIMEOUT="${RUN_TIMEOUT:-2}"
  mkdir -p /tmp/swift_test_config
  run_svr() {
    local name=$1 bin=$2 env_extra=${3:-}
    echo "===== $name ====="
    eval "export $env_extra" && timeout "$RUN_TIMEOUT" "$BUILD/$bin/$bin" "config/${name}.conf.example" 2>&1 | head -20
    local ret=$?
    if [[ $ret -eq 124 ]]; then echo "[OK] $name: config read and server started (timeout)"
    elif [[ $ret -eq 0 ]]; then echo "[OK] $name: exited normally"
    else echo "[FAIL] $name: exit $ret"; return $ret; fi
    echo ""
  }
  run_svr authsvr authsvr "AUTHSVR_ROCKSDB_PATH=/tmp/swift_test_config/auth"
  run_svr friendsvr friendsvr "FRIENDSVR_ROCKSDB_PATH=/tmp/swift_test_config/friend"
  mkdir -p /tmp/swift_test_config/chat
  run_svr chatsvr chatsvr "CHATSVR_ROCKSDB_PATH=/tmp/swift_test_config/chat"
  run_svr filesvr filesvr "FILESVR_ROCKSDB_PATH=/tmp/swift_test_config/file_meta FILESVR_STORAGE_PATH=/tmp/swift_test_config/file_storage"
  echo "All config load tests passed."
}

# ---------- 2. AuthSvr 测试 ----------
run_test_authsvr() {
  echo "===== test: AuthSvr ====="
  local AUTHSVR_BIN="${BIN_DIR:-build/backend/authsvr}/authsvr"
  local PORT="${AUTHSVR_PORT:-9094}"
  local DATA_DIR="${TMPDIR:-/tmp}/authsvr_test_$$"
  local GRPCURL_OPTS=(-plaintext -import-path backend/common/proto -import-path backend/authsvr/proto -proto backend/authsvr/proto/auth.proto)
  local SVC="swift.auth.AuthService"

  if [[ ! -x "$AUTHSVR_BIN" ]]; then
    echo "==> $AUTHSVR_BIN not found, building..."
    (cd build 2>/dev/null && cmake .. -DCMAKE_BUILD_TYPE=Debug && make authsvr) || \
    (mkdir -p build && (cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make authsvr))
  fi
  [[ -x "$AUTHSVR_BIN" ]] || { echo "Error: authsvr not found at $AUTHSVR_BIN" >&2; return 1; }

  mkdir -p "$DATA_DIR"
  export AUTHSVR_PORT="$PORT" AUTHSVR_ROCKSDB_PATH="$DATA_DIR"
  "$AUTHSVR_BIN" &
  local AUTHSVR_PID=$!
  cleanup() { kill -TERM "$AUTHSVR_PID" 2>/dev/null || true; wait "$AUTHSVR_PID" 2>/dev/null || true; rm -rf "$DATA_DIR"; }
  trap cleanup EXIT
  sleep 2
  kill -0 "$AUTHSVR_PID" 2>/dev/null || { echo "Error: AuthSvr exited early" >&2; return 1; }

  if ! command -v grpcurl >/dev/null 2>&1; then
    echo "grpcurl not installed; AuthSvr is running."; wait "$AUTHSVR_PID"; return 0
  fi
  local REG VERIFY CODE USER_ID UPD
  REG=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"username":"testuser","password":"pass1234","nickname":"Test"}' "localhost:$PORT" "$SVC/Register")
  echo "Register: $REG"
  CODE=$(echo "$REG" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "Register failed (code=$CODE)" >&2; return 1; }
  USER_ID=$(echo "$REG" | grep -oE '"(userId|user_id)":"[^"]*"' | head -1 | cut -d'"' -f4)

  VERIFY=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"username":"testuser","password":"pass1234"}' "localhost:$PORT" "$SVC/VerifyCredentials")
  CODE=$(echo "$VERIFY" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "VerifyCredentials failed (code=$CODE)" >&2; return 1; }

  if [[ -n "$USER_ID" ]]; then
    grpcurl "${GRPCURL_OPTS[@]}" -d "{\"user_id\":\"$USER_ID\"}" "localhost:$PORT" "$SVC/GetProfile" >/dev/null
    UPD=$(grpcurl "${GRPCURL_OPTS[@]}" -d "{\"user_id\":\"$USER_ID\",\"nickname\":\"Updated\"}" "localhost:$PORT" "$SVC/UpdateProfile")
    CODE=$(echo "$UPD" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
    [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "UpdateProfile failed (code=$CODE)" >&2; return 1; }
  fi
  echo "==> AuthSvr gRPC checks passed."
}

# ---------- 3. ZoneSvr 测试 ----------
run_test_zonesvr() {
  echo "===== test: ZoneSvr ====="
  local ZONESVR_BIN="${BIN_DIR:-build/backend/zonesvr}/zonesvr"
  local PORT="${ZONESVR_PORT:-9092}"
  local DATA_DIR="${TMPDIR:-/tmp}/zonesvr_test_$$"
  local CONF="$DATA_DIR/zonesvr.conf"
  local GRPCURL_OPTS=(-plaintext -import-path backend/common/proto -import-path backend/zonesvr/proto -proto backend/zonesvr/proto/zone.proto)
  local SVC="swift.zone.ZoneService"

  if [[ ! -x "$ZONESVR_BIN" ]]; then
    (cd build 2>/dev/null && cmake .. -DCMAKE_BUILD_TYPE=Debug && make zonesvr) || \
    (mkdir -p build && (cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make zonesvr))
  fi
  [[ -x "$ZONESVR_BIN" ]] || { echo "Error: zonesvr not found at $ZONESVR_BIN" >&2; return 1; }

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
  "$ZONESVR_BIN" "$CONF" &
  local ZONE_PID=$!
  cleanup() { kill -TERM "$ZONE_PID" 2>/dev/null || true; wait "$ZONE_PID" 2>/dev/null || true; rm -rf "$DATA_DIR"; }
  trap cleanup EXIT
  sleep 2
  kill -0 "$ZONE_PID" 2>/dev/null || { echo "Error: ZoneSvr exited early" >&2; return 1; }
  local i; for i in 1 2 3 4 5; do grpcurl -plaintext "127.0.0.1:$PORT" list >/dev/null 2>&1 && break; sleep 1; done

  if ! command -v grpcurl >/dev/null 2>&1; then echo "grpcurl not installed."; wait "$ZONE_PID"; return 0; fi
  local TARGET="127.0.0.1:$PORT" REG HB ON STATUS OFF STATUS2 CODE
  REG=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"gate_id":"gate_test","address":"localhost:9091","current_connections":0}' "$TARGET" "$SVC/GateRegister")
  CODE=$(echo "$REG" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "GateRegister failed (code=$CODE)" >&2; return 1; }
  HB=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"gate_id":"gate_test","current_connections":0}' "$TARGET" "$SVC/GateHeartbeat")
  CODE=$(echo "$HB" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "GateHeartbeat failed (code=$CODE)" >&2; return 1; }
  ON=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u1","gate_id":"gate_test","device_type":"web","device_id":"d1"}' "$TARGET" "$SVC/UserOnline")
  CODE=$(echo "$ON" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "UserOnline failed (code=$CODE)" >&2; return 1; }
  STATUS=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_ids":["u1"]}' "$TARGET" "$SVC/GetUserStatus")
  CODE=$(echo "$STATUS" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "GetUserStatus failed (code=$CODE)" >&2; return 1; }
  echo "$STATUS" | grep -qE '"online"\s*:\s*true' || { echo "GetUserStatus: u1 should be online" >&2; return 1; }
  OFF=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u1","gate_id":"gate_test"}' "$TARGET" "$SVC/UserOffline")
  CODE=$(echo "$OFF" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "UserOffline failed (code=$CODE)" >&2; return 1; }
  STATUS2=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_ids":["u1"]}' "$TARGET" "$SVC/GetUserStatus")
  echo "$STATUS2" | grep -qE '"online"\s*:\s*true' && { echo "GetUserStatus: u1 should be offline" >&2; return 1; }
  echo "==> ZoneSvr gRPC checks passed."
}

# ---------- 4. GateSvr 测试 ----------
run_test_gatesvr() {
  echo "===== test: GateSvr ====="
  local ZONESVR_BIN="${BIN_DIR:-build/backend/zonesvr}/zonesvr"
  local GATESVR_BIN="${BIN_DIR:-build/backend/gatesvr}/gatesvr"
  local ZONE_PORT="${ZONESVR_PORT:-9192}"
  local GATE_GRPC_PORT="${GATESVR_GRPC_PORT:-9191}"
  local GATE_WS_PORT="${GATESVR_WS_PORT:-9190}"
  local DATA_DIR="${TMPDIR:-/tmp}/gatesvr_test_$$"
  local ZONE_CONF="$DATA_DIR/zonesvr.conf" GATE_CONF="$DATA_DIR/gatesvr.conf"
  local GRPCURL_OPTS=(-plaintext -import-path backend/common/proto -import-path backend/gatesvr/proto -proto backend/gatesvr/proto/gate.proto)
  local SVC="swift.gate.GateInternalService"

  for name in zonesvr gatesvr; do
    local BIN="build/backend/$name/$name"
    if [[ ! -x "$BIN" ]]; then
      (cd build 2>/dev/null && cmake .. -DCMAKE_BUILD_TYPE=Debug && make zonesvr gatesvr) || \
      (mkdir -p build && (cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make zonesvr gatesvr))
    fi
  done
  [[ -x "$ZONESVR_BIN" ]] || { echo "Error: zonesvr not found" >&2; return 1; }
  [[ -x "$GATESVR_BIN" ]] || { echo "Error: gatesvr not found" >&2; return 1; }

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
  "$ZONESVR_BIN" "$ZONE_CONF" &
  local ZONE_PID=$!
  sleep 2
  kill -0 "$ZONE_PID" 2>/dev/null || { rm -rf "$DATA_DIR"; echo "Error: ZoneSvr exited early" >&2; return 1; }
  "$GATESVR_BIN" "$GATE_CONF" &
  local GATE_PID=$!
  cleanup() {
    kill -TERM "$GATE_PID" 2>/dev/null || true; wait "$GATE_PID" 2>/dev/null || true
    kill -TERM "$ZONE_PID" 2>/dev/null || true; wait "$ZONE_PID" 2>/dev/null || true
    rm -rf "$DATA_DIR"
  }
  trap cleanup EXIT
  sleep 2
  kill -0 "$GATE_PID" 2>/dev/null || { echo "Error: GateSvr exited early" >&2; return 1; }

  if ! command -v grpcurl >/dev/null 2>&1; then echo "grpcurl not installed."; wait "$GATE_PID"; return 0; fi
  local PUSH KICK CODE
  PUSH=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u_nobody","cmd":"test","payload":""}' "localhost:$GATE_GRPC_PORT" "$SVC/PushMessage")
  echo "$PUSH" | grep -q '"code"' || { echo "PushMessage: response should contain code" >&2; return 1; }
  KICK=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u_any","reason":"test kick"}' "localhost:$GATE_GRPC_PORT" "$SVC/DisconnectUser")
  CODE=$(echo "$KICK" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "DisconnectUser failed (code=$CODE)" >&2; return 1; }
  echo "==> GateSvr gRPC checks passed."
}

# ---------- 执行全部 ----------
run_test_config_load
run_test_authsvr
run_test_zonesvr
run_test_gatesvr
echo ""
echo "All tests passed."
