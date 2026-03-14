#!/usr/bin/env bash
# 单测入口：配置加载、AuthSvr、OnlineSvr、FriendSvr、ChatSvr、FileSvr、ZoneSvr、GateSvr。由 Makefile 的 make test 调用。
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

# ---------- 3. OnlineSvr 测试 ----------
run_test_onlinesvr() {
  echo "===== test: OnlineSvr ====="
  local ONLINESVR_BIN="${BIN_DIR:-build/backend/onlinesvr}/onlinesvr"
  local PORT="${ONLINESVR_PORT:-9095}"
  local DATA_DIR="${TMPDIR:-/tmp}/onlinesvr_test_$$"
  local GRPCURL_OPTS=(-plaintext -import-path backend/common/proto -import-path backend/onlinesvr/proto -proto backend/onlinesvr/proto/online.proto)
  local SVC="swift.online.OnlineService"

  if [[ ! -x "$ONLINESVR_BIN" ]]; then
    echo "==> $ONLINESVR_BIN not found, building..."
    (cd build 2>/dev/null && cmake .. -DCMAKE_BUILD_TYPE=Debug && make onlinesvr) || \
    (mkdir -p build && (cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make onlinesvr))
  fi
  [[ -x "$ONLINESVR_BIN" ]] || { echo "Error: onlinesvr not found at $ONLINESVR_BIN" >&2; return 1; }

  mkdir -p "$DATA_DIR"
  export ONLINESVR_PORT="$PORT" ONLINESVR_ROCKSDB_PATH="$DATA_DIR"
  "$ONLINESVR_BIN" &
  local ONLINE_PID=$!
  cleanup() { kill -TERM "$ONLINE_PID" 2>/dev/null || true; wait "$ONLINE_PID" 2>/dev/null || true; rm -rf "$DATA_DIR"; }
  trap cleanup EXIT
  sleep 2
  kill -0 "$ONLINE_PID" 2>/dev/null || { echo "Error: OnlineSvr exited early" >&2; return 1; }

  if ! command -v grpcurl >/dev/null 2>&1; then
    echo "grpcurl not installed; OnlineSvr is running."; wait "$ONLINE_PID"; return 0
  fi

  local LOGIN LOGOUT TOKEN VALIDATE CODE
  # 测试登录
  LOGIN=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u_test","device_id":"d1","device_type":"web"}' "localhost:$PORT" "$SVC/Login")
  echo "Login: $LOGIN"
  CODE=$(echo "$LOGIN" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "Login failed (code=$CODE)" >&2; return 1; }
  TOKEN=$(echo "$LOGIN" | grep -oE '"token":"[^"]*"' | head -1 | cut -d'"' -f4)

  # 测试验证 token
  if [[ -n "$TOKEN" ]]; then
    VALIDATE=$(grpcurl "${GRPCURL_OPTS[@]}" -d "{\"token\":\"$TOKEN\"}" "localhost:$PORT" "$SVC/ValidateToken")
    CODE=$(echo "$VALIDATE" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
    [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "ValidateToken failed (code=$CODE)" >&2; return 1; }
    echo "$VALIDATE" | grep -qE '"valid"\s*:\s*true' || { echo "ValidateToken: token should be valid" >&2; return 1; }
  fi

  # 测试登出
  if [[ -n "$TOKEN" ]]; then
    LOGOUT=$(grpcurl "${GRPCURL_OPTS[@]}" -d "{\"user_id\":\"u_test\",\"token\":\"$TOKEN\"}" "localhost:$PORT" "$SVC/Logout")
    CODE=$(echo "$LOGOUT" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
    [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "Logout failed (code=$CODE)" >&2; return 1; }
  fi

  echo "==> OnlineSvr gRPC checks passed."
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

# ---------- 4. FriendSvr 测试 ----------
run_test_friendsvr() {
  echo "===== test: FriendSvr ====="
  local FRIENDSVR_BIN="${BIN_DIR:-build/backend/friendsvr}/friendsvr"
  local PORT="${FRIENDSVR_PORT:-9096}"
  local DATA_DIR="${TMPDIR:-/tmp}/friendsvr_test_$$"
  local GRPCURL_OPTS=(-plaintext -import-path backend/common/proto -import-path backend/authsvr/proto -import-path backend/friendsvr/proto -proto backend/friendsvr/proto/friend.proto)
  local SVC="swift.relation.FriendService"

  if [[ ! -x "$FRIENDSVR_BIN" ]]; then
    echo "==> $FRIENDSVR_BIN not found, building..."
    (cd build 2>/dev/null && cmake .. -DCMAKE_BUILD_TYPE=Debug && make friendsvr) || \
    (mkdir -p build && (cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make friendsvr))
  fi
  [[ -x "$FRIENDSVR_BIN" ]] || { echo "Error: friendsvr not found at $FRIENDSVR_BIN" >&2; return 1; }

  mkdir -p "$DATA_DIR"
  export FRIENDSVR_PORT="$PORT" FRIENDSVR_ROCKSDB_PATH="$DATA_DIR"
  "$FRIENDSVR_BIN" &
  local FRIEND_PID=$!
  cleanup() { kill -TERM "$FRIEND_PID" 2>/dev/null || true; wait "$FRIEND_PID" 2>/dev/null || true; rm -rf "$DATA_DIR"; }
  trap cleanup EXIT
  sleep 2
  kill -0 "$FRIEND_PID" 2>/dev/null || { echo "Error: FriendSvr exited early" >&2; return 1; }

  if ! command -v grpcurl >/dev/null 2>&1; then
    echo "grpcurl not installed; FriendSvr is running."; wait "$FRIEND_PID"; return 0
  fi

  local ADD HANDLE GET BLOCK CODE
  # 测试添加好友
  ADD=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u1","friend_id":"u2","remark":"test friend"}' "localhost:$PORT" "$SVC/AddFriend")
  echo "AddFriend: $ADD"
  CODE=$(echo "$ADD" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "AddFriend failed (code=$CODE)" >&2; return 1; }

  # 测试获取好友列表
  GET=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u1"}' "localhost:$PORT" "$SVC/GetFriends")
  echo "GetFriends: $GET"
  CODE=$(echo "$GET" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "GetFriends failed (code=$CODE)" >&2; return 1; }

  # 测试拉黑用户
  BLOCK=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u1","target_id":"u3"}' "localhost:$PORT" "$SVC/BlockUser")
  CODE=$(echo "$BLOCK" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "BlockUser failed (code=$CODE)" >&2; return 1; }

  echo "==> FriendSvr gRPC checks passed."
}

# ---------- 5. ChatSvr 测试 ----------
run_test_chatsvr() {
  echo "===== test: ChatSvr ====="
  local CHATSVR_BIN="${BIN_DIR:-build/backend/chatsvr}/chatsvr"
  local PORT="${CHATSVR_PORT:-9097}"
  local DATA_DIR="${TMPDIR:-/tmp}/chatsvr_test_$$"
  local GRPCURL_OPTS=(-plaintext -import-path backend/common/proto -import-path backend/authsvr/proto -import-path backend/chatsvr/proto -proto backend/chatsvr/proto/chat.proto)
  local SVC="swift.chat.ChatService"

  if [[ ! -x "$CHATSVR_BIN" ]]; then
    echo "==> $CHATSVR_BIN not found, building..."
    (cd build 2>/dev/null && cmake .. -DCMAKE_BUILD_TYPE=Debug && make chatsvr) || \
    (mkdir -p build && (cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make chatsvr))
  fi
  [[ -x "$CHATSVR_BIN" ]] || { echo "Error: chatsvr not found at $CHATSVR_BIN" >&2; return 1; }

  mkdir -p "$DATA_DIR"
  export CHATSVR_PORT="$PORT" CHATSVR_ROCKSDB_PATH="$DATA_DIR"
  "$CHATSVR_BIN" &
  local CHAT_PID=$!
  cleanup() { kill -TERM "$CHAT_PID" 2>/dev/null || true; wait "$CHAT_PID" 2>/dev/null || true; rm -rf "$DATA_DIR"; }
  trap cleanup EXIT
  sleep 2
  kill -0 "$CHAT_PID" 2>/dev/null || { echo "Error: ChatSvr exited early" >&2; return 1; }

  if ! command -v grpcurl >/dev/null 2>&1; then
    echo "grpcurl not installed; ChatSvr is running."; wait "$CHAT_PID"; return 0
  fi

  local SEND PULL HISTORY MARK CODE
  # 测试发送消息
  SEND=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"from_user_id":"u1","to_id":"u2","chat_type":1,"content":"hello"}' "localhost:$PORT" "$SVC/SendMessage")
  echo "SendMessage: $SEND"
  CODE=$(echo "$SEND" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "SendMessage failed (code=$CODE)" >&2; return 1; }

  # 测试拉取离线消息
  PULL=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u2","limit":10}' "localhost:$PORT" "$SVC/PullOffline")
  echo "PullOffline: $PULL"
  CODE=$(echo "$PULL" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "PullOffline failed (code=$CODE)" >&2; return 1; }

  # 测试获取历史消息
  HISTORY=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"chat_id":"u2","chat_type":1,"user_id":"u1","limit":10}' "localhost:$PORT" "$SVC/GetHistory")
  echo "GetHistory: $HISTORY"
  CODE=$(echo "$HISTORY" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "GetHistory failed (code=$CODE)" >&2; return 1; }

  echo "==> ChatSvr gRPC checks passed."
}

# ---------- 6. FileSvr 测试 ----------
run_test_filesvr() {
  echo "===== test: FileSvr ====="
  local FILESVR_BIN="${BIN_DIR:-build/backend/filesvr}/filesvr"
  local PORT="${FILESVR_PORT:-9098}"
  local DATA_DIR="${TMPDIR:-/tmp}/filesvr_test_$$"
  local STORAGE_DIR="$DATA_DIR/storage"
  local GRPCURL_OPTS=(-plaintext -import-path backend/common/proto -import-path backend/filesvr/proto -proto backend/filesvr/proto/file.proto)
  local SVC="swift.file.FileService"

  if [[ ! -x "$FILESVR_BIN" ]]; then
    echo "==> $FILESVR_BIN not found, building..."
    (cd build 2>/dev/null && cmake .. -DCMAKE_BUILD_TYPE=Debug && make filesvr) || \
    (mkdir -p build && (cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make filesvr))
  fi
  [[ -x "$FILESVR_BIN" ]] || { echo "Error: filesvr not found at $FILESVR_BIN" >&2; return 1; }

  mkdir -p "$DATA_DIR" "$STORAGE_DIR"
  export FILESVR_PORT="$PORT" FILESVR_ROCKSDB_PATH="$DATA_DIR" FILESVR_STORAGE_PATH="$STORAGE_DIR"
  "$FILESVR_BIN" &
  local FILE_PID=$!
  cleanup() { kill -TERM "$FILE_PID" 2>/dev/null || true; wait "$FILE_PID" 2>/dev/null || true; rm -rf "$DATA_DIR" "$STORAGE_DIR"; }
  trap cleanup EXIT
  sleep 2
  kill -0 "$FILE_PID" 2>/dev/null || { echo "Error: FileSvr exited early" >&2; return 1; }

  if ! command -v grpcurl >/dev/null 2>&1; then
    echo "grpcurl not installed; FileSvr is running."; wait "$FILE_PID"; return 0
  fi

  local INIT UPLOAD TOKEN INFO CODE
  # 测试初始化上传
  INIT=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u1","file_name":"test.txt","content_type":"text/plain","file_size":1024}' "localhost:$PORT" "$SVC/InitUpload")
  echo "InitUpload: $INIT"
  CODE=$(echo "$INIT" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "InitUpload failed (code=$CODE)" >&2; return 1; }

  # 测试获取上传凭证
  TOKEN=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"u1","file_name":"test.txt","file_size":1024}' "localhost:$PORT" "$SVC/GetUploadToken")
  echo "GetUploadToken: $TOKEN"
  CODE=$(echo "$TOKEN" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  [[ -z "$CODE" || "$CODE" == "0" ]] || { echo "GetUploadToken failed (code=$CODE)" >&2; return 1; }

  echo "==> FileSvr gRPC checks passed."
}

# ---------- 7. GateSvr 测试 ----------
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

# ---------- 执行全部测试 ----------
run_test_config_load
run_test_authsvr
run_test_onlinesvr
run_test_friendsvr
run_test_chatsvr
run_test_filesvr
run_test_zonesvr
run_test_gatesvr
echo ""
echo "All tests passed."
