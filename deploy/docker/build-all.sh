#!/usr/bin/env bash
# 在仓库根目录执行，构建所有后端镜像
set -e
cd "$(dirname "$0")/../.."

REGISTRY="${REGISTRY:-swift}"
TAG="${TAG:-latest}"

for target in authsvr onlinesvr friendsvr chatsvr filesvr zonesvr gatesvr; do
  echo "Building $REGISTRY/$target:$TAG ..."
  docker build -f deploy/docker/Dockerfile --build-arg BUILD_TARGET=$target -t $REGISTRY/$target:$TAG .
done
echo "Done. Images: $REGISTRY/<authsvr|onlinesvr|friendsvr|chatsvr|filesvr|zonesvr|gatesvr>:$TAG"
