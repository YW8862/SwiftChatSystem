#!/usr/bin/env bash
# 在仓库根目录执行，直接在 Minikube 内部 Docker 环境构建，彻底解决 latest 冲突和多余镜像问题。
# 若 not found 可试: BASE_IMAGE=registry.cn-hangzhou.aliyuncs.com/library/ubuntu:22.04
set -e
cd "$(dirname "$0")/../.."

# 开启 Minikube 内部 Docker 环境
# 这样不仅能省去极其漫长的 image load 过程，还能避免宿主机和 Minikube 之间的镜像版本不同步！
if command -v minikube &>/dev/null && minikube status &>/dev/null; then
  echo "=> Minikube is running. Switching to Minikube docker environment..."
  eval $(minikube -p minikube docker-env)
else
  echo "=> Warning: Minikube is not running. Building on host Docker..."
  unset DOCKER_HOST DOCKER_CERT_PATH DOCKER_TLS_VERIFY MINIKUBE_ACTIVE_DOCKERD
fi

REGISTRY="${REGISTRY:-swift}"
TAG="latest" # 回归 latest，我们通过直接在 Minikube 内部覆盖并重启 Pod 来解决
BASE_IMAGE="${BASE_IMAGE:-docker.1ms.run/library/ubuntu:22.04}"

for target in authsvr onlinesvr friendsvr chatsvr filesvr zonesvr gatesvr; do
  img="$REGISTRY/$target:$TAG"
  echo "========================================="
  echo "Building $img ..."
  echo "========================================="
  
  # 直接在目标 Docker 守护进程（Minikube 内部）中构建
  # 这样最新构建出的镜像直接就在 Minikube 里，覆盖了原有的 latest
  docker build -f deploy/docker/Dockerfile \
    --build-arg BUILD_TARGET=$target \
    --build-arg BASE_IMAGE="$BASE_IMAGE" \
    -t "$img" .
  
  # 将 k8s 部署文件恢复为使用 latest 标签（防止你之前执行了上一个版本的脚本）
  sed -i "s|image: $REGISTRY/$target:.*|image: $img|g" "deploy/k8s/${target}-deployment.yaml"
done

echo "=> Applying k8s configurations..."
kubectl apply -k ./deploy/k8s

echo "=> Restarting pods to force them to pull the newly built local 'latest' image..."
# 因为 rollout restart --all 在某些版本报错，这里直接删除所有旧 Pod，
# Deployment 会立刻自动重建它们。由于 yaml 配置了 imagePullPolicy: Never，
# 新 Pod 启动时会立刻使用刚才在 Minikube 内部构建好的那个最新的 latest 镜像！
kubectl delete pods --all -n master

echo "=> Cleaning up old unused images to save disk space..."
# 一键清理刚才由于重新构建 latest，导致失去标签变成 <none>:<none> 的废弃旧镜像。
# 这行命令就是帮你释放硬盘空间的“终极魔法”！
docker image prune -f

echo "=> All Done! Use 'kubectl get pods -n master' to check status."