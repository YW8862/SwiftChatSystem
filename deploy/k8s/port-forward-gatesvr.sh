#!/usr/bin/env bash
# 将宿主机 9090 端口转发到 Minikube 集群内 gatesvr 的 9090 端口（WebSocket）。
# 用法：
#   ./port-forward-gatesvr.sh           # 仅本机 localhost:9090
#   ./port-forward-gatesvr.sh --all     # 监听 0.0.0.0:9090，允许其他机器访问
set -e
NAMESPACE="${NAMESPACE:-master}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ "${1:-}" == "--all" ]]; then
  echo "Forwarding 0.0.0.0:9090 -> gatesvr:9090 (namespace=$NAMESPACE)"
  exec kubectl port-forward --address 0.0.0.0 -n "$NAMESPACE" svc/gatesvr 9090:9090
else
  echo "Forwarding localhost:9090 -> gatesvr:9090 (namespace=$NAMESPACE)"
  exec kubectl port-forward -n "$NAMESPACE" svc/gatesvr 9090:9090
fi
