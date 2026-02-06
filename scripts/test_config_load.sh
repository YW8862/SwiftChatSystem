#!/usr/bin/env bash
# 测试各 svr 能否正确读取 config（使用 config/*.conf.example + 环境变量覆盖数据目录到 /tmp）
set -e
cd "$(dirname "$0")/.."
BUILD="${BUILD:-./build/backend}"
RUN_TIMEOUT="${RUN_TIMEOUT:-2}"
mkdir -p /tmp/swift_test_config

run_svr() {
  local name=$1
  local bin=$2
  local env_extra=${3:-}
  echo "===== $name ====="
  eval "export $env_extra" && timeout "$RUN_TIMEOUT" "$BUILD/$bin/$bin" "config/${name}.conf.example" 2>&1 | head -20
  local ret=$?
  if [[ $ret -eq 124 ]]; then
    echo "[OK] $name: config read and server started (timeout)"
  elif [[ $ret -eq 0 ]]; then
    echo "[OK] $name: exited normally"
  else
    echo "[FAIL] $name: exit $ret"
    return $ret
  fi
  echo ""
}

run_svr authsvr authsvr "AUTHSVR_ROCKSDB_PATH=/tmp/swift_test_config/auth"
run_svr friendsvr friendsvr "FRIENDSVR_ROCKSDB_PATH=/tmp/swift_test_config/friend"
mkdir -p /tmp/swift_test_config/chat
run_svr chatsvr chatsvr "CHATSVR_ROCKSDB_PATH=/tmp/swift_test_config/chat"
run_svr filesvr filesvr "FILESVR_ROCKSDB_PATH=/tmp/swift_test_config/file_meta FILESVR_STORAGE_PATH=/tmp/swift_test_config/file_storage"

echo "All config load tests passed."
