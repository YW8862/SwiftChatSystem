#!/usr/bin/env bash
# 在仓库根目录执行，构建所有后端镜像
# 默认国内镜像，避免 Docker Hub 超时
# 若 not found 可试: BASE_IMAGE=registry.cn-hangzhou.aliyuncs.com/library/ubuntu:22.04 或 ubuntu:22.04
set -e
cd "$(dirname "$0")/../.."

REGISTRY="${REGISTRY:-swift}"
TAG="${TAG:-latest}"
BASE_IMAGE="${BASE_IMAGE:-docker.1ms.run/library/ubuntu:22.04}"

for target in authsvr onlinesvr friendsvr chatsvr filesvr zonesvr gatesvr; do
  echo "Building $REGISTRY/$target:$TAG ..."
  docker build -f deploy/docker/Dockerfile \
    --build-arg BUILD_TARGET=$target \
    --build-arg BASE_IMAGE="$BASE_IMAGE" \
    -t $REGISTRY/$target:$TAG .
done
echo "Done. Images: $REGISTRY/<authsvr|onlinesvr|friendsvr|chatsvr|filesvr|zonesvr|gatesvr>:$TAG"
