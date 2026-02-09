#!/usr/bin/env bash
# 在仓库根目录执行，构建所有后端镜像
# 默认使用国内镜像源，避免 Docker Hub 超时；海外环境可: export BASE_IMAGE=ubuntu:22.04
set -e
cd "$(dirname "$0")/../.."

REGISTRY="${REGISTRY:-swift}"
TAG="${TAG:-latest}"
BASE_IMAGE="${BASE_IMAGE:-registry.cn-hangzhou.aliyuncs.com/library/ubuntu:22.04}"

for target in authsvr onlinesvr friendsvr chatsvr filesvr zonesvr gatesvr; do
  echo "Building $REGISTRY/$target:$TAG ..."
  docker build -f deploy/docker/Dockerfile \
    --build-arg BUILD_TARGET=$target \
    --build-arg BASE_IMAGE="$BASE_IMAGE" \
    -t $REGISTRY/$target:$TAG .
done
echo "Done. Images: $REGISTRY/<authsvr|onlinesvr|friendsvr|chatsvr|filesvr|zonesvr|gatesvr>:$TAG"
