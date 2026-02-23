#!/bin/bash
# 仅构建 SwiftChatSystem 服务端（不构建客户端）
# 依赖: 本机已安装 gRPC、Protobuf、RocksDB、Boost、OpenSSL 等（可自行用系统包或 vcpkg 安装）
set -e
cd "$(dirname "$0")/.."
PROJECT_ROOT=$(pwd)

BUILD_DIR="${1:-build}"
echo "=== 构建服务端（输出目录: $BUILD_DIR）==="

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DSWIFT_BUILD_CLIENT=OFF \
  -DBUILD_AUTHSVR_TESTS=OFF \
  -DBUILD_FRIENDSVR_TESTS=OFF \
  -DBUILD_CHATSVR_TESTS=OFF \
  -DBUILD_FILESVR_TESTS=OFF

make -j$(nproc)

echo ""
echo "=== 服务端构建完成 ==="
echo "可执行文件位于: $PROJECT_ROOT/$BUILD_DIR/backend/"
echo "  authsvr, onlinesvr, gatesvr, zonesvr, friendsvr, chatsvr, filesvr"
