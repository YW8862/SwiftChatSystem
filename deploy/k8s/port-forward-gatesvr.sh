#!/usr/bin/env bash
# 将宿主机 9090 端口转发到 Minikube 集群内 gatesvr 的 9090 端口（WebSocket）。
# 用法：
#   ./port-forward-gatesvr.sh           # 仅本机 localhost:9090
#   ./port-forward-gatesvr.sh --all     # 监听 0.0.0.0:9090，允许其他机器访问
set -euo pipefail
NAMESPACE="${NAMESPACE:-master}"
LISTEN_ADDR="127.0.0.1"
if [[ "${1:-}" == "--all" ]]; then
  LISTEN_ADDR="0.0.0.0"
fi

require_bin() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "缺少命令: $1" >&2
    exit 1
  fi
}

kubectl_ok() {
  kubectl version --request-timeout=5s >/dev/null 2>&1
}

ensure_cluster_ready() {
  if kubectl_ok; then
    return 0
  fi

  echo "kubectl 当前无法连接 Kubernetes，尝试自动修复..." >&2

  if command -v minikube >/dev/null 2>&1; then
    minikube update-context >/dev/null 2>&1 || true
    if ! kubectl_ok; then
      echo "尝试启动 minikube（首次可能较慢）..." >&2
      minikube start >/dev/null
      minikube update-context >/dev/null 2>&1 || true
    fi
  fi

  if ! kubectl_ok; then
    echo "仍无法连接 Kubernetes API，请先确认集群状态：" >&2
    echo "  - kubectl config get-contexts" >&2
    echo "  - minikube status / minikube start" >&2
    exit 1
  fi
}

require_bin kubectl
ensure_cluster_ready

if ! kubectl get ns "$NAMESPACE" >/dev/null 2>&1; then
  echo "命名空间不存在: $NAMESPACE" >&2
  exit 1
fi

if ! kubectl -n "$NAMESPACE" get svc gatesvr >/dev/null 2>&1; then
  echo "服务不存在: svc/gatesvr (namespace=$NAMESPACE)" >&2
  exit 1
fi

if [[ "$LISTEN_ADDR" == "0.0.0.0" ]]; then
  echo "Forwarding 0.0.0.0:9090 -> gatesvr:9090 (namespace=$NAMESPACE)"
  exec kubectl port-forward --address 0.0.0.0 -n "$NAMESPACE" svc/gatesvr 9090:9090
else
  echo "Forwarding localhost:9090 -> gatesvr:9090 (namespace=$NAMESPACE)"
  exec kubectl port-forward -n "$NAMESPACE" svc/gatesvr 9090:9090
fi
