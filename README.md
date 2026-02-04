# SwiftChatSystem

高性能可扩展社交平台 —— C++ 微服务架构

## 项目概述

构建一个功能完整、高性能、可演示的类 QQ 实时社交系统。

### 核心功能

- 私聊 / 群聊 / 好友关系 / 富媒体消息
- **统一会话模型**：私聊视为仅两人的会话（type=private），两好友间唯一一个私聊会话；群聊为多人会话（type=group），同一批人可建多个群。消息统一按 conversation_id 存储；拉人/踢人等仅对群聊开放。
- 群聊：创建群至少 3 人（不允许 1 人或 2 人建群），现有群支持邀请非成员进群
- 消息撤回 / @提醒 / 已读回执 / 离线推送 / 消息搜索
- Windows 客户端（Qt5）
- Minikube 本地 Kubernetes 部署

认证与登录：**AuthSvr** 负责注册、校验用户名密码（VerifyCredentials）、用户资料；**OnlineSvr** 负责登录会话、Token 签发与校验（Login/Logout/ValidateToken）。详见 `system.md`、`develop.md`。

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
| 存储     | RocksDB                       |
| 日志     | AsyncLogger（自研）           |
| 构建     | CMake + vcpkg                 |
| 部署     | Minikube + Kubernetes         |
| 客户端   | Qt 5.15                       |

## 快速开始

### 1. 安装依赖

```bash
# 使用 vcpkg（需指定工具链）
vcpkg install grpc protobuf rocksdb boost-beast openssl nlohmann-json
# 或从项目根目录：cmake -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

### 2. 构建项目

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
make -j$(nproc)
```

### 3. 部署到 Minikube

```bash
# 启动 Minikube
minikube start --driver=docker

# 部署服务
kubectl apply -k deploy/k8s/

# 获取客户端连接地址
minikube service gatesvr --url
```

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

## 开发进度

- [x] 阶段 1：基础设施（Proto、AsyncLogger、common 公共库与 JWT）
- [x] 阶段 2：AuthSvr（注册、VerifyCredentials、资料）、OnlineSvr（登录会话、JWT）
- [ ] 阶段 3：FriendSvr / ChatSvr / FileSvr 业务服务
- [ ] 阶段 4：ZoneSvr 路由与 GateSvr 接入
- [ ] 阶段 5：Qt 客户端
- [ ] 阶段 6：容器化与 K8s 部署

## License

MIT
