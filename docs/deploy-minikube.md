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

**方式一：将本机 9090 端口转发到 gatesvr（推荐）**

若希望在本机直接使用 `ws://127.0.0.1:9090/ws` 连接（与 docker-compose 一致），可用 `kubectl port-forward` 将宿主机 9090 转发到集群内 gatesvr 的 9090：

```bash
# 在项目 deploy/k8s 目录或任意处执行（需已部署且 gatesvr Pod 正常）
./deploy/k8s/port-forward-gatesvr.sh
# 或直接：
kubectl port-forward -n swift-chat svc/gatesvr 9090:9090
```

保持该终端运行；客户端连接 `ws://127.0.0.1:9090/ws` 即可。若需允许其他机器访问本机 9090，可：

```bash
./deploy/k8s/port-forward-gatesvr.sh --all
# 或：kubectl port-forward --address 0.0.0.0 -n swift-chat svc/gatesvr 9090:9090
```

**方式二：通过 NodePort 访问**

Gate 的 Service 已配置为 NodePort，端口 30090：

```bash
minikube service -n swift-chat gatesvr --url
# 会输出类似 http://192.168.49.2:30090；WebSocket 实际为 ws://<该 IP>:30090
```

浏览器或客户端连接：`ws://<minikube ip>:30090`（NodePort 为 30090 时）。

**方式三：minikube tunnel（LoadBalancer）**

```bash
minikube tunnel
# 另开终端
kubectl -n swift-chat get svc gatesvr   # 查看 NodePort 或 LoadBalancer
```

### 6.2 查看日志与排错

```bash
kubectl -n swift-chat logs -f deployment/gatesvr
kubectl -n swift-chat describe pod -l app=zonesvr
```

---

## 七、常见问题排查

### 7.1 kubeadm 报错 "cannot open html" / "Syntax error: redirection unexpected"

说明 Minikube 下载的 Kubernetes 组件**不是真正的二进制**，而是被替换成了 HTML（例如网络劫持、代理错误页、下载未完成）。集群内的 `kubeadm` 实为 HTML 文件，被当作脚本执行就会报上述错误。

**处理步骤：**

1. **删除当前集群并清理缓存**（让 Minikube 重新下载组件）：
   ```bash
   minikube delete
   minikube delete --all --purge
   ```
   若仍有问题，可手动删缓存后再启动：
   ```bash
   rm -rf ~/.minikube/cache
   minikube start --cpus=2 --memory=4096 --driver=docker
   ```

2. **检查网络与代理**：
   - 若使用 HTTP 代理，确保 `HTTP_PROXY`/`HTTPS_PROXY` 正确，且不会把 `dl.k8s.io`、`storage.googleapis.com` 等返回成 HTML 错误页。
   - 若在国内网络，可尝试使用镜像或代理后再执行 `minikube start`。

3. **指定较旧且可能已缓存的 Kubernetes 版本**（避免重新拉取失败）：
   ```bash
   minikube start --cpus=2 --memory=4096 --driver=docker --kubernetes-version=v1.28.0
   ```

4. **确认本机可直接下载到真实二进制**（可选）：
   ```bash
   curl -sL "https://dl.k8s.io/release/v1.28.0/bin/linux/amd64/kubeadm" -o /tmp/kubeadm
   file /tmp/kubeadm   # 应显示 "ELF 64-bit LSB executable"
   ```
   若这里是 HTML，说明当前网络环境下 K8s 官方地址被重定向到错误页，需先解决网络或改用镜像。

### 7.2 基础镜像 / 组件下载很慢

若 Minikube 从 GitHub 或 Google 拉取 kicbase 等镜像很慢，可：

- 使用已有镜像（若曾成功拉取过）：Minikube 会复用本地/缓存镜像。
- 配置 Docker 镜像加速或 HTTP 代理后再执行 `minikube start`。

---

## 八、停止与清理

```bash
# 删除本项目的 K8s 资源（保留命名空间也可）
kubectl delete -k deploy/k8s

# 停止 Minikube 集群
minikube stop

# 删除集群（可选）
minikube delete
```

---

## 九、简要流程汇总

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

# 5. 等待 Pod 就绪后，将本机 9090 转发到 gatesvr（可选，便于客户端用 ws://127.0.0.1:9090/ws）
kubectl -n swift-chat get pods
./deploy/k8s/port-forward-gatesvr.sh   # 或：minikube service -n swift-chat gatesvr --url 用 NodePort
```

客户端使用 port-forward 时连接 `ws://127.0.0.1:9090/ws`；使用 NodePort 时连接 `ws://<minikube service --url 输出的 host>:30090`。
