# SwiftChatSystem 部署

- **云服务器从零部署**：参见 **[docs/deploy-to-server.md](../docs/deploy-to-server.md)**，按步骤完成环境准备、构建镜像与 Docker Compose 或本机运行。
- **本地 Kubernetes（Minikube）**：参见 **[docs/deploy-minikube.md](../docs/deploy-minikube.md)**，使用 Minikube 创建集群并部署全部后端服务。

---

## 构建镜像

依赖（gRPC、Protobuf、RocksDB 等）通过 vcpkg 在镜像内安装，**构建上下文必须为仓库根目录**。

### 方式一：统一 Dockerfile（推荐）

```bash
# 在仓库根目录执行
cd /path/to/SwiftChatSystem

# 构建单个服务（以 zonesvr 为例）
docker build -f deploy/docker/Dockerfile --build-arg BUILD_TARGET=zonesvr -t swift/zonesvr:latest .

# 构建全部 7 个后端服务
chmod +x deploy/docker/build-all.sh
./deploy/docker/build-all.sh
```

可选环境变量：`REGISTRY`（默认 `swift`）、`TAG`（默认 `latest`）。

### 方式二：各服务独立 Dockerfile

```bash
docker build -f backend/authsvr/Dockerfile -t swift/authsvr:latest .
docker build -f backend/onlinesvr/Dockerfile -t swift/onlinesvr:latest .
docker build -f backend/friendsvr/Dockerfile -t swift/friendsvr:latest .
docker build -f backend/chatsvr/Dockerfile -t swift/chatsvr:latest .
docker build -f backend/filesvr/Dockerfile -t swift/filesvr:latest .
docker build -f backend/zonesvr/Dockerfile -t swift/zonesvr:latest .
docker build -f backend/gatesvr/Dockerfile -t swift/gatesvr:latest .
```

## Kubernetes 部署

**使用 Minikube 在本地建集群**：完整步骤见 **[docs/deploy-minikube.md](../docs/deploy-minikube.md)**（安装 Minikube、创建 hostPath、构建/导入镜像、应用配置、访问服务）。

### 前置条件

- 集群可访问镜像：`swift/<service>:latest`（本地构建后需推送到集群可拉取的仓库，或使用 Minikube 加载：`minikube image load swift/<service>:latest`，并设置 `imagePullPolicy: Never`）
- 已创建 PVC：`swift-data-pvc`（RocksDB 等）、`swift-files-pvc`（文件存储）；Minikube 下需在节点上预先创建 hostPath 目录（见 Minikube 文档）

### 通用部署步骤

```bash
# 若使用 Minikube，先加载本地镜像（详见 docs/deploy-minikube.md）
for s in authsvr onlinesvr friendsvr chatsvr filesvr zonesvr gatesvr; do
  minikube image load swift/$s:latest
done

# 应用 k8s 配置（含 namespace、configmap、pv/pvc、各服务 Deployment+Service）
kubectl apply -k deploy/k8s

# 查看
kubectl -n swift-chat get pods
kubectl -n swift-chat get svc
```

### 服务与端口

| 服务     | 端口（集群内） | 说明           |
|----------|----------------|----------------|
| gatesvr  | 9090 (WS), 9091 (gRPC) | NodePort 30090 (WS) |
| zonesvr  | 9092           | 路由/会话      |
| authsvr  | 9094           | 认证/资料      |
| onlinesvr| 9095           | 登录/Token     |
| friendsvr| 9096           | 好友           |
| chatsvr  | 9098           | 消息/群组      |
| filesvr  | 9100 (gRPC), 8080 (HTTP) | NodePort 30080 (HTTP) |

配置通过 ConfigMap `swift-config` 注入；内网密钥等敏感项建议用 Secret 覆盖（如 `ZONESVR_INTERNAL_SECRET`、`GATESVR_ZONESVR_INTERNAL_SECRET`）。

### 使用 Redis 会话存储（可选）

若需多副本 Zone 共享会话，部署 Redis 并修改 ConfigMap：

- `ZONESVR_SESSION_STORE_TYPE: "redis"`
- `ZONESVR_REDIS_URL: "redis://<redis-service>:6379"`

然后重启 zonesvr。
