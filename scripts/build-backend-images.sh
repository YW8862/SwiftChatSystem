#!/usr/bin/env bash
# 构建所有后端 Docker 镜像（在仓库根目录执行）
# 用法: ./scripts/build-backend-images.sh
set -e
cd "$(dirname "$0")/.."
exec ./deploy/docker/build-all.sh
