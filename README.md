# SwiftChatSystem

高性能可扩展社交平台 —— C++ 微服务架构

## 项目概述

构建一个功能完整、高性能、可演示的类 QQ 实时社交系统。

### 核心功能

- 私聊 / 群聊 / 好友关系 / 富媒体消息
- 消息撤回 / @提醒 / 已读回执 / 离线推送 / 消息搜索
- Windows 客户端（Qt5）
- Minikube 本地 Kubernetes 部署

## 项目结构

```
SwiftChatSystem/
├── backend/                    # 后端服务
│   ├── proto/                  # Protobuf 协议定义
│   ├── common/                 # 公共库
│   ├── gatesvr/               # 接入网关（WebSocket）
│   ├── zonesvr/               # 路由服务（在线状态）
│   ├── authsvr/               # 认证服务
│   ├── friendsvr/             # 好友服务
│   ├── chatsvr/               # 消息服务
│   └── filesvr/               # 文件服务
├── client/                     # 客户端
│   ├── proto/                  # 客户端协议
│   └── desktop/               # Qt 桌面客户端
├── deploy/                     # 部署配置
│   └── k8s/                   # Kubernetes YAML
├── libs/                       # 第三方库
└── docs/                       # 文档
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
# 使用 vcpkg
vcpkg install grpc protobuf rocksdb boost-beast openssl
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
| GateSvr    | 9090      | WebSocket: 9090   |
| ZoneSvr    | 9092      | -                 |
| AuthSvr    | 9094      | -                 |
| FriendSvr  | 9096      | -                 |
| ChatSvr    | 9098      | -                 |
| FileSvr    | 9100      | HTTP: 8080        |

## 开发进度

- [ ] 阶段 1：基础设施（Proto、AsyncLogger）
- [ ] 阶段 2：认证与好友服务
- [ ] 阶段 3：消息核心服务
- [ ] 阶段 4：接入与路由层
- [ ] 阶段 5：Qt 客户端
- [ ] 阶段 6：容器化与部署

## License

MIT
