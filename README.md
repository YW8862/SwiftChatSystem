# SwiftChatSystem

高性能可扩展社交平台 —— C++ 微服务架构

## 项目概述

构建一个功能完整、高性能、可演示的类 QQ 实时社交系统。

### 核心功能

- 私聊 / 群聊 / 好友关系 / 富媒体消息
- **统一会话模型**：私聊视为仅两人的会话（type=private），两好友间唯一一个私聊会话；群聊为多人会话（type=group），同一批人可建多个群。消息统一按 conversation_id 存储；拉人/踢人等仅对群聊开放。
- 群聊：创建群至少 3 人（不允许 1 人或 2 人建群），现有群支持邀请非成员进群
- 消息撤回 / @提醒 / 已读回执 / 离线推送 / 消息搜索（客户端本地）
- Windows 客户端（Qt5）
- Minikube 本地 Kubernetes 部署

**请求链路（Gate → Zone → System → 后端 gRPC）：** 客户端通过 WebSocket 连接 **GateSvr**；业务请求由 GateSvr 经 gRPC 转发到 **ZoneSvr**，ZoneSvr 根据 cmd 分发到对应 **System**（AuthSystem、ChatSystem、FriendSystem、GroupSystem、FileSystem），各 System **通过 gRPC 调用后端业务服务**（AuthSvr、ChatSvr、FriendSvr、FileSvr 等）执行业务逻辑，结果经 Zone → Gate 返回客户端。详见 `docs/develop.md` 9.6、`docs/system.md` 2.2。

认证与登录：**AuthSvr** 负责注册、校验用户名密码（VerifyCredentials）、用户资料；**OnlineSvr** 负责登录会话、Token 签发与校验（Login/Logout/ValidateToken）。  
**API 鉴权**：涉及「当前用户」的接口（AuthSvr 的 GetProfile/UpdateProfile、FriendSvr、ChatSvr 全部接口）须在 gRPC metadata 中携带 JWT（`authorization: Bearer <token>` 或 `x-token: <token>`），服务端以 Token 解析出的 user_id 为准，不信任请求体中的 user_id，防止越权。详见 `develop.md` 第 16 节、`system.md` 2.3。

## 项目结构

```
SwiftChatSystem/
├── backend/                    # 后端服务
│   ├── common/                 # 公共库（utils、jwt_helper、proto 等）
│   ├── authsvr/                # 认证服务（注册、VerifyCredentials、资料）
│   ├── onlinesvr/              # 登录会话服务（Login/Logout/ValidateToken、JWT）
│   ├── zonesvr/                 # 路由服务（在线状态、消息路由）
│   ├── gatesvr/                 # 接入网关（WebSocket）
│   ├── friendsvr/               # 好友服务
│   ├── chatsvr/                 # 消息与群组服务（MessageStore/ConversationRegistry/GroupStore）
│   └── filesvr/                 # 文件服务
├── client/                     # 客户端
│   ├── proto/                  # 客户端协议
│   └── desktop/                # Qt 桌面客户端
├── component/                  # 内部组件（如 AsyncLogger）
├── deploy/                     # 部署配置
│   └── k8s/                    # Kubernetes YAML
└── develop.md / system.md      # 开发与架构文档
```

## 技术栈

| 类别     | 技术                          |
| -------- | ----------------------------- |
| 语言     | C++17                         |
| RPC      | gRPC + Protobuf               |
| 网络     | Boost.Beast (WebSocket)       |
| 存储     | RocksDB（后续可扩展 Redis/MySQL 集群） |
| 日志     | AsyncLogger（自研，双缓冲异步写） |
| 构建     | CMake + vcpkg                 |
| 部署     | Minikube + Kubernetes         |
| 客户端   | Qt 5.15                       |

> **写入路径**：当前 ChatSvr、FileSvr 等**直接同步写**（不经过异步写中间层）。`swift::async::IWriteExecutor`（`swift/async_write.h`）已实现并预留，待引入真正异步写（如 Redis/MySQL 集群下的 `AsyncDbWriteExecutor`）时，再由业务注入并改用 `SubmitAndWait` / `Submit`。

## 部署方式

本项目支持三种部署方式：

1. **本地部署（源码编译 + 本机运行）**：适合开发与调试。
2. **Docker Compose 单机部署**：适合单台服务器一键部署。
3. **Kubernetes 集群部署（以 Minikube 为例）**：适合本地模拟生产集群。

下面给出每种方式的概览，详细步骤请参考对应文档。

### 1. 本地部署（源码编译 + 本机运行）

适用场景：本地开发、调试、单机演示。

- **步骤概览**（详细见 `docs/deploy-to-server.md` 中“本机编译并运行”部分）：

```bash
# 1）安装构建依赖（以 Ubuntu 为例）
sudo apt-get update
sudo apt-get install -y build-essential cmake git curl unzip tar pkg-config

# 2）安装 vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh -disableMetrics
export VCPKG_ROOT=$(pwd)

# 3）在项目根目录编译
cd /path/to/SwiftChatSystem
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
make -j$(nproc)
```

- **配置与启动**：

```bash
# 从示例复制配置（按需修改）
cd /path/to/SwiftChatSystem
cp config/authsvr.conf.example    config/authsvr.conf
cp config/onlinesvr.conf.example  config/onlinesvr.conf
cp config/friendsvr.conf.example  config/friendsvr.conf
cp config/chatsvr.conf.example    config/chatsvr.conf
cp config/filesvr.conf.example    config/filesvr.conf
cp config/zonesvr.conf.example    config/zonesvr.conf
cp config/gatesvr.conf.example    config/gatesvr.conf

# 按依赖顺序在 build 目录启动（可各开一终端）
cd build
./backend/authsvr/authsvr       ../config/authsvr.conf &
./backend/onlinesvr/onlinesvr   ../config/onlinesvr.conf &
./backend/friendsvr/friendsvr   ../config/friendsvr.conf &
./backend/chatsvr/chatsvr       ../config/chatsvr.conf &
./backend/filesvr/filesvr       ../config/filesvr.conf &
./backend/zonesvr/zonesvr       ../config/zonesvr.conf &
./backend/gatesvr/gatesvr       ../config/gatesvr.conf &
```

> 更多细节（系统依赖、systemd 管理、云服务器本机部署等），请查看：`docs/deploy-to-server.md` 的“部署方式二：本机编译并运行”。

### 2. Docker Compose 单机部署

适用场景：一台物理机/云服务器上快速拉起全部 7 个服务，依赖通过 Docker 镜像统一管理。

- **前置条件**：已安装 Docker 与 Docker Compose 插件（见 `docs/deploy-to-server.md` 中“安装 Docker 与 Docker Compose”）。

- **构建镜像并启动**（在项目根目录）：

```bash
cd /path/to/SwiftChatSystem

# 1）构建全部镜像（首次会比较慢）
chmod +x deploy/docker/build-all.sh
./deploy/docker/build-all.sh

# 2）使用 docker-compose 启动所有服务
docker compose -f deploy/docker-compose.yml up -d

# 3）查看状态
docker compose -f deploy/docker-compose.yml ps
```

- **停止/重启**：

```bash
# 停止（保留数据卷）
docker compose -f deploy/docker-compose.yml down

# 仅重启网关
docker compose -f deploy/docker-compose.yml restart gatesvr
```

> 详细的云服务器部署（包括防火墙、安全组、内网密钥配置等），请参考：`docs/deploy-to-server.md` 中“部署方式一：Docker Compose（推荐）”。

### 3. 集群部署（Minikube 示例）

适用场景：在本地模拟 Kubernetes 生产环境，验证多副本、服务发现与负载均衡。

- **前置条件**：已安装 `kubectl` 与 `minikube`（见 `docs/deploy-minikube.md` 第 1 章）。

- **快速流程概览**：

```bash
cd /path/to/SwiftChatSystem

# 1）启动 Minikube（建议至少 2 CPU / 4GB 内存）
minikube start --cpus=2 --memory=4096 --driver=docker

# 2）在 Minikube 节点上创建数据目录（hostPath）
minikube ssh -- "sudo mkdir -p /data/swift-chat /data/swift-files && sudo chmod 777 /data/swift-chat /data/swift-files"

# 3）构建并导入镜像
./deploy/docker/build-all.sh
for s in authsvr onlinesvr friendsvr chatsvr filesvr zonesvr gatesvr; do
  minikube image load swift/$s:latest
done

# 4）应用 K8s 配置
kubectl apply -k deploy/k8s

# 5）等待 Pod 就绪 & 获取 GateSvr 地址
kubectl -n swift-chat get pods
minikube service -n swift-chat gatesvr --url
```

客户端将 WebSocket 地址配置为上述输出的 `ws://<host>:<port>` 即可连接（通常为 NodePort 30090）。

> 集群部署的详细步骤、排错指南与常见问题，请参考：`docs/deploy-minikube.md`。

## 服务端口

| 服务       | gRPC 端口 | 其他端口          |
| ---------- | --------- | ----------------- |
| GateSvr    | 9091      | WebSocket: 9090   |
| ZoneSvr    | 9092      | -                 |
| AuthSvr    | 9094      | -                 |
| OnlineSvr  | 9095      | -                 |
| FriendSvr  | 9096      | -                 |
| ChatSvr    | 9098      | -                 |
| FileSvr    | 9100      | HTTP: 8080        |

## License

MIT
