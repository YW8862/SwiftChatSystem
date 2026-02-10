#!/bin/bash
VERSION=${1:-v1.35.0}
ARCH=amd64
CACHE_DIR="$HOME/.minikube/cache/linux/$ARCH/$VERSION"

echo "🔧 Preparing Kubernetes binaries for $VERSION..."

mkdir -p "$CACHE_DIR"
cd "$CACHE_DIR"

for bin in kubelet kubeadm kubectl; do
  if [ ! -f "$bin" ]; then
    echo "📥 Downloading $bin from Tsinghua mirror..."
    curl -L -o "$bin" "https://mirrors.tuna.tsinghua.edu.cn/github-release/kubernetes/kubernetes/LatestRelease/$bin"
    chmod +x "$bin"
  else
    echo "✅ $bin already exists."
  fi
done

echo "✅ Done! Now run your minikube start command."