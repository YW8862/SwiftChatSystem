#!/usr/bin/env bash
# 测试 AuthSvr main 功能：启动服务并用 grpcurl 调用 Register / VerifyCredentials / GetProfile / UpdateProfile
# 依赖：grpcurl（可选，未安装时仅启动服务并打印手动测试命令）
# 用法：从项目根目录执行 ./scripts/test_authsvr.sh

set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

AUTHSVR_BIN="${BIN_DIR:-bin}/authsvr"
PORT="${AUTHSVR_PORT:-9094}"
DATA_DIR="${TMPDIR:-/tmp}/authsvr_test_$$"
GRPCURL_OPTS=(-plaintext -import-path backend/common/proto -import-path backend/authsvr/proto -proto backend/authsvr/proto/auth.proto)
SVC="swift.auth.AuthService"

# 若没有可执行文件则尝试构建
if [[ ! -x "$AUTHSVR_BIN" ]]; then
  echo "==> $AUTHSVR_BIN not found, building..."
  make build 2>/dev/null || {
    mkdir -p build && (cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make authsvr)
    mkdir -p bin
    cp -f build/backend/authsvr/authsvr bin/ 2>/dev/null || true
  }
fi
if [[ ! -x "$AUTHSVR_BIN" ]]; then
  echo "Error: authsvr binary not found at $AUTHSVR_BIN" >&2
  exit 1
fi

mkdir -p "$DATA_DIR"
export AUTHSVR_PORT="$PORT"
export AUTHSVR_ROCKSDB_PATH="$DATA_DIR"

echo "==> Starting AuthSvr (port=$PORT, data=$DATA_DIR)..."
"$AUTHSVR_BIN" &
AUTHSVR_PID=$!
cleanup() {
  kill -TERM "$AUTHSVR_PID" 2>/dev/null || true
  wait "$AUTHSVR_PID" 2>/dev/null || true
  rm -rf "$DATA_DIR"
}
trap cleanup EXIT

sleep 2
if ! kill -0 "$AUTHSVR_PID" 2>/dev/null; then
  echo "Error: AuthSvr process exited early" >&2
  exit 1
fi

echo "==> Testing gRPC (grpcurl)..."
if ! command -v grpcurl >/dev/null 2>&1; then
  echo "grpcurl not installed. Server is running; manual test commands:"
  echo "  grpcurl ${GRPCURL_OPTS[*]} -d '{\"username\":\"testuser\",\"password\":\"pass1234\",\"nickname\":\"Test\"}' localhost:$PORT $SVC/Register"
  echo "  grpcurl ${GRPCURL_OPTS[*]} -d '{\"username\":\"testuser\",\"password\":\"pass1234\"}' localhost:$PORT $SVC/VerifyCredentials"
  echo "  grpcurl ${GRPCURL_OPTS[*]} -d '{\"user_id\":\"<user_id>\"}' localhost:$PORT $SVC/GetProfile"
  wait "$AUTHSVR_PID"
  exit 0
fi

# 1. Register（成功：无 code 或 code=0；user_id 可能是 userId 或 user_id）
REG=$(grpcurl "${GRPCURL_OPTS[@]}" -d '{"username":"testuser","password":"pass1234","nickname":"Test"}' "localhost:$PORT" "$SVC/Register")
echo "Register: $REG"
CODE=$(echo "$REG" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
if [[ -n "$CODE" && "$CODE" != "0" ]]; then
  echo "Register failed (code=$CODE)" >&2
  exit 1
fi
USER_ID=$(echo "$REG" | grep -oE '"(userId|user_id)":"[^"]*"' | head -1 | cut -d'"' -f4)
[[ -z "$USER_ID" ]] && echo "Warning: could not parse user_id from Register response"

# 2. VerifyCredentials（成功：无 code 或 code=0）
VERIFY=$(grpcurl "${GRPCURL_OPTS[@]}" -d "{\"username\":\"testuser\",\"password\":\"pass1234\"}" "localhost:$PORT" "$SVC/VerifyCredentials")
echo "VerifyCredentials: $VERIFY"
CODE=$(echo "$VERIFY" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
if [[ -n "$CODE" && "$CODE" != "0" ]]; then
  echo "VerifyCredentials failed (code=$CODE)" >&2
  exit 1
fi

# 3. GetProfile（若未拿到 user_id 则用占位，可能 404）
if [[ -n "$USER_ID" ]]; then
  PROFILE=$(grpcurl "${GRPCURL_OPTS[@]}" -d "{\"user_id\":\"$USER_ID\"}" "localhost:$PORT" "$SVC/GetProfile")
  echo "GetProfile: $PROFILE"
fi

# 4. UpdateProfile
if [[ -n "$USER_ID" ]]; then
  UPD=$(grpcurl "${GRPCURL_OPTS[@]}" -d "{\"user_id\":\"$USER_ID\",\"nickname\":\"Updated\"}" "localhost:$PORT" "$SVC/UpdateProfile")
  echo "UpdateProfile: $UPD"
  CODE=$(echo "$UPD" | grep -oE '"code":[0-9]+' | head -1 | cut -d: -f2)
  if [[ -n "$CODE" && "$CODE" != "0" ]]; then
    echo "UpdateProfile failed (code=$CODE)" >&2
    exit 1
  fi
fi

echo "==> All gRPC checks passed."
exit 0
