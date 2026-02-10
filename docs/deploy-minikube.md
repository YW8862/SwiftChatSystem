# 使用 Minikube 部署到本地 Kubernetes 集群

本文档说明如何用 **Minikube** 在本地创建 Kubernetes 集群，并部署 SwiftChatSystem 全部 7 个后端服务。

---

## 一、安装 Minikube 与 kubectl

### 1.1 安装 kubectl

```bash
# Linux (以 x86_64 为例)
curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl"
chmod +x kubectl
sudo mv kubectl /usr/local/bin/
kubectl version --client
```

### 1.2 安装 Minikube

```bash
# Linux x86_64
curl -LO https://storage.googleapis.com/minikube/releases/latest/minikube-linux-amd64
sudo install minikube-linux-amd64 /usr/local/bin/minikube
minikube version
```

或使用包管理器（若可用）：

```bash
# 例如 Debian/Ubuntu
curl -LO https://storage.googleapis.com/minikube/releases/latest/minikube_latest_amd64.deb
sudo dpkg -i minikube_latest_amd64.deb
```

---

## 二、启动 Minikube 集群

### 2.1 创建集群（建议分配足够内存）

```bash
# 建议至少 4GB 内存、2 核，以便跑齐 7 个服务
minikube start --cpus=2 --memory=4096 --driver=docker
```

常用 driver：`docker`（本机已装 Docker 时）、`none`（本机即 Linux 宿主机时）。其他可选 `virtualbox`、`podman` 等。

### 2.2 确认集群就绪

```bash
kubectl cluster-info
kubectl get nodes
```

---

## 三、准备存储目录（hostPath）

当前 K8s 配置使用 **hostPath** 持久化数据（RocksDB、文件存储）。Minikube 的“节点”即 Minikube 虚拟机，需在虚拟机内创建对应目录：

```bash
minikube ssh
sudo mkdir -p /data/swift-chat /data/swift-files
sudo chmod 777 /data/swift-chat /data/swift-files
exit
```

---

## 四、构建镜像并导入 Minikube

Minikube 使用自带的 Docker 环境，需在**本机**构建好镜像后**导入**到 Minikube，或使用 Minikube 的 Docker 环境在“集群内”构建。

### 4.1 方式 A：本机构建后导入（推荐）

在项目根目录执行：

```bash
# 1. 构建全部镜像（使用当前 Docker）
./deploy/docker/build-all.sh

# 2. 将镜像导入 Minikube（避免集群去外网拉镜像）
for s in authsvr onlinesvr friendsvr chatsvr filesvr zonesvr gatesvr; do
  minikube image load swift/$s:latest
done
```

### 4.2 方式 B：在 Minikube 的 Docker 内构建

```bash
# 使用 Minikube 的 Docker，构建的镜像直接进集群
eval $(minikube docker-env)
./deploy/docker/build-all.sh
eval $(minikube docker-env -u)   # 可选：切回本机 Docker
```

---

## 五、部署到集群

### 5.1 应用 K8s 配置

在**项目根目录**执行：

```bash
kubectl apply -k deploy/k8s
```

会创建命名空间 `swift-chat`、ConfigMap、PV/PVC、各服务 Deployment 与 Service。

### 5.2 查看状态

```bash
kubectl -n swift-chat get pods
kubectl -n swift-chat get svc
```

等待所有 Pod 为 `Running`（首次拉取/启动可能需 1～2 分钟）。

### 5.3 使用本地镜像（避免 ImagePullBackOff）

若未推送到镜像仓库，需让 Deployment 使用本地已加载的镜像。当前配置未写 `imagePullPolicy`，默认会按 `IfNotPresent` 使用已存在的 `swift/*:latest`。若出现 `ImagePullBackOff`，可为各 Deployment 增加 `imagePullPolicy: Never`（或通过 Kustomize patch）。下面用一条命令为 `swift-chat` 命名空间内所有 Deployment 打补丁：

```bash
# 为命名空间内所有 Deployment 设置 imagePullPolicy: Never（使用 minikube 内镜像）
kubectl -n swift-chat patch deployment authsvr -p '{"spec":{"template":{"spec":{"containers":[{"name":"authsvr","imagePullPolicy":"Never"}]}}}}'
kubectl -n swift-chat patch deployment onlinesvr -p '{"spec":{"template":{"spec":{"containers":[{"name":"onlinesvr","imagePullPolicy":"Never"}]}}}}'
kubectl -n swift-chat patch deployment friendsvr -p '{"spec":{"template":{"spec":{"containers":[{"name":"friendsvr","imagePullPolicy":"Never"}]}}}}'
kubectl -n swift-chat patch deployment chatsvr -p '{"spec":{"template":{"spec":{"containers":[{"name":"chatsvr","imagePullPolicy":"Never"}]}}}}'
kubectl -n swift-chat patch deployment filesvr -p '{"spec":{"template":{"spec":{"containers":[{"name":"filesvr","imagePullPolicy":"Never"}]}}}}'
kubectl -n swift-chat patch deployment zonesvr -p '{"spec":{"template":{"spec":{"containers":[{"name":"zonesvr","imagePullPolicy":"Never"}]}}}}'
kubectl -n swift-chat patch deployment gatesvr -p '{"spec":{"template":{"spec":{"containers":[{"name":"gatesvr","imagePullPolicy":"Never"}]}}}}'
```

也可在 `deploy/k8s` 下为各 Deployment 的 `containers[].imagePullPolicy` 统一设为 `Never`，再 `kubectl apply -k deploy/k8s`。

---

## 六、访问服务

### 6.1 WebSocket（客户端连接 Gate）

Gate 的 Service 已配置为 NodePort，端口 30090：

```bash
minikube service -n swift-chat gatesvr --url
# 会输出类似 http://192.168.49.2:30090；WebSocket 实际为 ws://<该 IP>:30090
```

或使用隧道（在宿主机直接访问 localhost:9090）：

```bash
minikube tunnel
# 另开终端
kubectl -n swift-chat get svc gatesvr   # 查看 NodePort 或 LoadBalancer
```

浏览器或客户端连接：`ws://<minikube ip>:30090`（NodePort 为 30090 时）。

### 6.2 查看日志与排错

```bash
kubectl -n swift-chat logs -f deployment/gatesvr
kubectl -n swift-chat describe pod -l app=zonesvr
```

---

## 七、停止与清理

```bash
# 删除本项目的 K8s 资源（保留命名空间也可）
kubectl delete -k deploy/k8s

# 停止 Minikube 集群
minikube stop

# 删除集群（可选）
minikube delete
```

---

## 八、简要流程汇总

```bash
# 1. 安装并启动 Minikube
minikube start --cpus=2 --memory=4096 --driver=docker

# 2. 在 Minikube 节点上创建 hostPath 目录
minikube ssh -- "sudo mkdir -p /data/swift-chat /data/swift-files && sudo chmod 777 /data/swift-chat /data/swift-files"

# 3. 构建并导入镜像（在项目根目录）
./deploy/docker/build-all.sh
for s in authsvr onlinesvr friendsvr chatsvr filesvr zonesvr gatesvr; do minikube image load swift/$s:latest; done

# 4. 部署
kubectl apply -k deploy/k8s

# 5. 等待 Pod 就绪后，获取 Gate 的访问地址
kubectl -n swift-chat get pods
minikube service -n swift-chat gatesvr --url
```

客户端将 WebSocket 地址配置为上述输出的 `ws://<host>:30090` 即可连接。
