# SwiftChatSystem 新手学习指南

> 从零开始学习这个类 QQ 的 C++ 微服务社交系统

## 📚 目录

1. [项目概述](#1-项目概述)
2. [学习路线图](#2-学习路线图)
3. [环境搭建](#3-环境搭建)
4. [架构详解](#4-架构详解)
5. [代码导读](#5-代码导读)
6. [实践任务](#6-实践任务)
7. [常见问题](#7-常见问题)

---

## 1. 项目概述

### 1.1 这是什么？

SwiftChatSystem 是一个**高性能、可扩展的实时社交系统**，采用 C++17 编写，架构设计类似微信/QQ 的后端系统。

**核心能力**：
- ✅ 私聊/群聊消息收发
- ✅ 好友关系管理
- ✅ 富媒体消息（图片/文件）
- ✅ 消息撤回、@提醒、已读回执
- ✅ JWT 认证鉴权
- ✅ Kubernetes 集群部署

### 1.2 技术栈总览

```
┌─────────────────────────────────────┐
│         技术栈金字塔                 │
├─────────────────────────────────────┤
│ 应用层   │ Qt 客户端 + WebSocket    │
│ 业务层   │ 7 个微服务 (gRPC)        │
│ 数据层   │ RocksDB/Redis/MySQL      │
│ 基础层   │ C++17 + Boost.Beast      │
└─────────────────────────────────────┘
```

| 类别 | 技术选型 | 作用 |
|------|---------|------|
| **语言** | C++17 | 核心编程语言 |
| **RPC** | gRPC + Protobuf | 服务间通信 |
| **网络** | Boost.Beast | WebSocket 服务器 |
| **存储** | RocksDB/MySQL | 数据持久化 |
| **缓存** | Redis | 会话状态 |
| **日志** | AsyncLogger(自研) | 异步日志系统 |
| **构建** | CMake + vcpkg | 依赖管理 |
| **部署** | Docker + K8s | 容器化编排 |

### 1.3 服务列表

```
┌─────────────────────────────────────────────────────────┐
│                    服务架构图                            │
│                                                          │
│  Client (WebSocket:9090)                                │
│      ↓                                                   │
│  ┌──────────┐                                            │
│  │ GateSvr  │ ← 接入网关 (WebSocket 连接管理)            │
│  │ :9091    │                                            │
│  └────┬─────┘                                            │
│       │ gRPC                                               │
│       ↓                                                   │
│  ┌──────────┐                                            │
│  │ ZoneSvr  │ ← API 网关 (请求路由 + 会话管理)            │
│  │ :9092    │                                            │
│  └────┬─────┘                                            │
│       ├──────────┬──────────┬──────────┬─────────┐      │
│       ↓          ↓          ↓          ↓         ↓      │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐│
│  │AuthSvr │ │OnlineSvr│ │FriendSvr│ │ChatSvr │ │FileSvr ││
│  │:9094   │ │:9095   │ │:9096   │ │:9098   │ │:9100   ││
│  └────────┘ └────────┘ └────────┘ └────────┘ └────────┘│
│                                                          │
└─────────────────────────────────────────────────────────┘
```

| 服务 | 端口 | 职责 | 类比 |
|------|------|------|------|
| **GateSvr** | WS:9090, gRPC:9091 | WebSocket 连接管理 | Nginx |
| **ZoneSvr** | gRPC:9092 | 请求路由 + 在线状态 | API Gateway |
| **AuthSvr** | gRPC:9094 | 用户注册/登录验证 | 认证中心 |
| **OnlineSvr** | gRPC:9095 | 登录会话 + JWT 签发 | Session 管理 |
| **FriendSvr** | gRPC:9096 | 好友关系管理 | 好友服务 |
| **ChatSvr** | gRPC:9098 | 消息存储与分发 | 消息服务 |
| **FileSvr** | gRPC:9100, HTTP:8080 | 文件上传下载 | 文件服务 |

---

## 2. 学习路线图

### 阶段 1：入门基础（1-2 周）

```
Week 1: 环境搭建 + 代码浏览
├── 安装依赖 (vcpkg, CMake, Boost)
├── 编译项目 (build 目录生成可执行文件)
├── 阅读 README.md 和 system.md
└── 理解目录结构

Week 2: 核心概念理解
├── 学习 gRPC + Protobuf 基础
├── 理解 WebSocket 协议
├── 掌握 JWT 认证原理
└── 了解 RocksDB 基础
```

### 阶段 2：深入理解（2-3 周）

```
Week 3-4: 服务拆解学习
├── GateSvr: WebSocket 连接管理
├── ZoneSvr: Zone-System 架构
├── AuthSvr + OnlineSvr: 认证流程
├── FriendSvr + ChatSvr: 业务逻辑
└── FileSvr: 文件处理

Week 5: 存储与消息
├── 统一会话模型（私聊 vs 群聊）
├── 消息存储设计（RocksDB Key 规范）
├── 离线消息队列
└── 已读回执机制
```

### 阶段 3：实战演练（1-2 周）

```
Week 6-7: 动手实践
├── 启动全部服务（本地运行）
├── 使用客户端测试功能
├── 添加新功能（如：表情包支持）
├── 性能优化实践
└── Docker 容器化部署
```

---

## 3. 环境搭建

### 3.1 基础依赖安装

```bash
# Ubuntu 22.04 示例
sudo apt-get update
sudo apt-get install -y build-essential cmake git curl unzip tar pkg-config \
                        libboost-all-dev libssl-dev protobuf-compiler \
                        libprotobuf-dev libgrpc-dev libgrpc++-dev \
                        libgflags-dev libgoogle-glog-dev \
                        librocksdb-dev libqt5webkit5-dev qtbase5-dev
```

### 3.2 vcpkg 安装

```bash
# 克隆 vcpkg
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg

# 初始化 bootstrap
./bootstrap-vcpkg.sh -disableMetrics

# 设置环境变量（添加到 ~/.bashrc）
export VCPKG_ROOT=~/vcpkg
export PATH=$VCPKG_ROOT:$PATH
```

### 3.3 编译项目

```bash
cd /home/yw/SwiftChatSystem

# 创建 build 目录
mkdir -p build && cd build

# CMake 配置
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# 编译（-j 参数根据 CPU 核心数调整）
make -j$(nproc)

# 编译完成后，build 目录下会生成所有服务的可执行文件
ls -lh backend/*/
```

### 3.4 配置服务

```bash
cd /home/yw/SwiftChatSystem

# 从示例配置复制
cp config/authsvr.conf.example    config/authsvr.conf
cp config/onlinesvr.conf.example  config/onlinesvr.conf
cp config/friendsvr.conf.example  config/friendsvr.conf
cp config/chatsvr.conf.example    config/chatsvr.conf
cp config/filesvr.conf.example    config/filesvr.conf
cp config/zonesvr.conf.example    config/zonesvr.conf
cp config/gatesvr.conf.example    config/gatesvr.conf

# 修改配置（可选，默认配置即可运行）
vim config/gatesvr.conf  # 可修改 WebSocket 端口等
```

### 3.5 启动服务

```bash
# 方式一：手动逐个启动（学习推荐）
cd build

# 按依赖顺序启动（每个服务开一个终端）
./backend/authsvr/authsvr     ../config/authsvr.conf &
./backend/onlinesvr/onlinesvr ../config/onlinesvr.conf &
./backend/friendsvr/friendsvr ../config/friendsvr.conf &
./backend/chatsvr/chatsvr     ../config/chatsvr.conf &
./backend/filesvr/filesvr     ../config/filesvr.conf &
./backend/zonesvr/zonesvr     ../config/zonesvr.conf &
./backend/gatesvr/gatesvr     ../config/gatesvr.conf &

# 方式二：使用脚本一键启动
./scripts/run-tests.sh  # 参考脚本中的启动顺序
```

### 3.6 验证服务

```bash
# 检查端口监听
netstat -tlnp | grep -E '909[0-2]|909[4-6]|9098|9100|8080'

# 预期输出：
# tcp  0  0 0.0.0.0:9090  LISTEN  # GateSvr WebSocket
# tcp  0  0 0.0.0.0:9091  LISTEN  # GateSvr gRPC
# tcp  0  0 0.0.0.0:9092  LISTEN  # ZoneSvr
# tcp  0  0 0.0.0.0:9094  LISTEN  # AuthSvr
# tcp  0  0 0.0.0.0:9095  LISTEN  # OnlineSvr
# tcp  0  0 0.0.0.0:9096  LISTEN  # FriendSvr
# tcp  0  0 0.0.0.0:9098  LISTEN  # ChatSvr
# tcp  0  0 0.0.0.0:9100  LISTEN  # FileSvr
# tcp  0  0 0.0.0.0:8080  LISTEN  # FileSvr HTTP
```

---

## 4. 架构详解

### 4.1 请求流转全流程

以「用户 A 发送消息给用户 B」为例：

```
┌──────────────────────────────────────────────────────────────┐
│                      消息发送流程                             │
└──────────────────────────────────────────────────────────────┘

Client A                GateSvr              ZoneSvr
   │                       │                     │
   │ ① chat.send_message   │                     │
   │ (WebSocket:9090)      │                     │
   ├──────────────────────>│                     │
   │                       │                     │
   │                       │ ② ForwardRequest    │
   │                       │ (gRPC:9092)         │
   │                       ├────────────────────>│
   │                       │                     │
   │                       │                     │ ③ 查询 SessionStore
   │                       │                     │ Bob 在哪个 Gate?
   │                       │                     │ 
   │                       │                     │ ④ ChatSystem.SendMessage
   │                       │                     │ (存储消息到 ChatSvr)
   │                       │                     ├──────────┐
   │                       │                     │          │
   │                       │                     │          ↓
   │                       │                     │     ChatSvr
   │                       │                     │   (RocksDB 存储)
   │                       │                     │
   │                       │                     │ ⑤ RouteToUser
   │                       │                     │ (如果 Bob 在线)
   │                       │<────────────────────┤
   │                       │                     │
   │ ⑥ 发送成功确认         │                     │
   │<──────────────────────┤                     │
   │                       │                     │
   │                       │                     │
   
   Client B (如果在线)
         ↑
         │ ⑦ chat.message 推送
         │ (通过 Bob 连接的 Gate)
         │
   ┌─────┴──────────┐
   │    Gate-X      │ (Bob 连接的网关)
   └────────────────┘
```

**代码位置对应**：

| 步骤 | 代码文件 | 关键函数 |
|------|---------|---------|
| ① | `client/desktop/src/` | 客户端发送逻辑 |
| ② | `backend/gatesvr/service/gate_service.cpp` | `ForwardToZone()` |
| ③ | `backend/zonesvr/store/session_store.h` | `GetSession(user_id)` |
| ④ | `backend/zonesvr/internal/system/chat_system.cpp` | `SendMessage()` |
| ⑤ | `backend/zonesvr/service/zone_service.cpp` | `RouteToUser()` |
| ⑥ | `backend/gatesvr/handler/websocket_handler.cpp` | `OnMessageSent()` |
| ⑦ | `backend/gatesvr/service/gate_service.cpp` | `PushToConnection()` |

### 4.2 Zone-System 架构解析

ZoneSvr 是核心路由层，采用 **API Gateway + 微服务**模式：

```cpp
// backend/zonesvr/internal/system/system_manager.h
class SystemManager {
    // 管理所有子系统
    std::unique_ptr<AuthSystem> auth_system_;
    std::unique_ptr<ChatSystem> chat_system_;
    std::unique_ptr<FriendSystem> friend_system_;
    std::unique_ptr<GroupSystem> group_system_;
    std::unique_ptr<FileSystem> file_system_;
    
    // 会话状态存储（内存/Redis）
    std::shared_ptr<SessionStore> session_store_;
};
```

**各 System 的职责**：

```
┌─────────────────────────────────────────────────────────┐
│                    ZoneSvr 内部结构                      │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Handler 层 (接收 Gate 请求)                             │
│      ↓                                                  │
│  SystemManager (分发到对应 System)                       │
│      ├── AuthSystem ──(RPC)──→ AuthSvr                  │
│      ├── ChatSystem ──(RPC)──→ ChatSvr                  │
│      ├── FriendSystem ──(RPC)──→ FriendSvr              │
│      ├── GroupSystem ──(RPC)──→ ChatSvr(GroupService)   │
│      └── FileSystem ──(RPC)──→ FileSvr                  │
│                                                         │
│  SessionStore (共享会话状态)                             │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

**为什么这样设计？**

1. **统一入口**：所有请求经过 ZoneSvr，便于鉴权、限流、监控
2. **RPC 客户端集中管理**：连接池、超时重试统一配置
3. **会话共享**：SessionStore 存储在线状态，多台 Zone 可共享
4. **职责清晰**：Zone 只转发，业务逻辑在后端 Svr

### 4.3 统一会话模型

**核心思想**：私聊和群聊统一抽象为「会话」(Conversation)

```cpp
// 会话类型定义
enum class ConversationType {
    PRIVATE = 1,  // 私聊（仅 2 人）
    GROUP = 2     // 群聊（≥3 人）
};

// 私聊会话 ID 生成规则
std::string GetOrCreatePrivateConversation(user1, user2) {
    // 格式：p_<min_user_id>_<max_user_id>
    // 示例：p_u_1001_u_1003
    return "p_" + std::min(user1, user2) + "_" 
               + std::max(user1, user2);
}

// 群聊会话 ID = group_id
// 示例：g_10001
```

**优势**：
- ✅ 消息存储统一按 `conversation_id` 组织
- ✅ 拉取历史消息逻辑一致
- ✅ 群专属能力（踢人、公告）通过 type 区分

### 4.4 JWT 认证流程

```
┌─────────────────────────────────────────────────────────┐
│              JWT 签发与验证流程                           │
└─────────────────────────────────────────────────────────┘

Client                          AuthSvr        OnlineSvr
   │                               │                │
   │ ① Register(username, pwd)     │                │
   ├──────────────────────────────>│                │
   │                               │                │
   │ ② VerifyCredentials()         │                │
   ├──────────────────────────────>│                │
   │    (校验用户名密码)            │                │
   │                               │                │
   │ ③ Login(user_id, device_id)   │                │
   ├───────────────────────────────┼───────────────>│
   │                               │                │
   │                               │                │ ④ GenerateToken()
   │                               │                │ (JWT HS256 签名)
   │                               │                │
   │                               │                │ ⑤ Write Session
   │                               │                │ (user_id → token)
   │                               │                │
   │ ⑥ Return token + expire_at    │                │
   │<──────────────────────────────┼────────────────┤
   │                               │                │
   │                               │                │
   │ ⑦ WebSocket Connect           │                │
   │    (建立长连接)                │                │
   │───────────────────────────────┼────────────────┼─────> GateSvr
   │                               │                │
   │ ⑧ auth.login(token)           │                │
   │    (绑定连接与用户)            │                │
   │───────────────────────────────┼────────────────┼─────> GateSvr
   │                               │                │
   │                               │                │ ⑨ ValidateToken()
   │                               │                │ (查 Session + 验签)
   │<──────────────────────────────┼────────────────┤
   │    (认证成功，连接已绑定)       │                │
   │                               │                │
   │ ⑩ 业务请求 (friend.add 等)     │                │
   │───────────────────────────────┼────────────────┼─────> GateSvr
   │                               │                │
   │                               │                │ ⑪ 后端 RPC metadata
   │                               │                │ 携带 token
   │                               │                │ (JwtVerify 本地验签)
   │<──────────────────────────────┼────────────────┤
   │    (业务处理完成)               │                │
   │                               │                │
```

**关键点**：
1. **签发方**：OnlineSvr 在 Login 时调用 `JwtCreate(user_id, secret)` 生成 Token
2. **两种验证**：
   - WebSocket 登录：查 OnlineSvr 的 SessionStore（判断是否仍有效）
   - 业务接口：本地 `JwtVerify(token, secret)`（不查库，纯计算）
3. **单设备策略**：同一用户在同一设备登录会复用 token，跨设备登录会拒绝

**代码位置**：

| 环节 | 文件路径 | 关键函数 |
|------|---------|---------|
| JWT 签发 | `backend/common/src/jwt_helper.cpp` | `JwtCreate()` |
| JWT 验签 | `backend/common/src/jwt_helper.cpp` | `JwtVerify()` |
| Login | `backend/onlinesvr/internal/service/online_service.cpp` | `Login()` |
| ValidateToken | `backend/onlinesvr/internal/service/online_service.cpp` | `ValidateToken()` |
| 业务鉴权 | `backend/common/src/grpc_auth.cpp` | `GetAuthenticatedUserId()` |

---

## 5. 代码导读

### 5.1 GateSvr 核心代码

**文件**：`backend/gatesvr/cmd/main.cpp`

```cpp
// 第 157-260 行：GateSvr 主函数
int main(int argc, char* argv[]) {
    // 1. 加载配置文件
    std::string config_file = (argc > 1) ? argv[1] : "";
    swift::gate::GateConfig config = swift::gate::LoadConfig(config_file);
    
    // 2. 初始化日志系统
    swift::log::InitFromEnv("gatesvr");
    
    // 3. 创建 GateService（核心业务逻辑）
    auto gate_svc = std::make_shared<swift::gate::GateService>();
    gate_svc->Init(config);
    
    // 4. 创建 WebSocket 处理器
    auto ws_handler = std::make_shared<swift::gate::WebSocketHandler>(gate_svc);
    
    // 5. 启动 gRPC 服务（监听 9091，供 ZoneSvr 回调）
    grpc::ServerBuilder builder;
    builder.AddListeningPort(grpc_addr, grpc::InsecureServerCredentials());
    auto grpc_server = builder.BuildAndStart();
    
    // 6. 等待 ZoneSvr 就绪并注册
    WaitForZoneSvrReady(config.zone_svr_addr, 30, 2);
    RegisterGateWithRetry(gate_svc, register_addr, 60);
    
    // 7. 启动 WebSocket 监听（9090）
    ws_listener->Run();
    
    // 8. 心跳线程（每 30s 上报存活 + 检测客户端超时）
    std::thread heartbeat_thread([&]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            gate_svc->RegisterGate(register_addr);  // 幂等重试
            gate_svc->CheckHeartbeat();              // 踢出超时客户端
            gate_svc->GateHeartbeat();               // 向 ZoneSvr 上报
        }
    });
    
    // 9. 主线程等待 gRPC 退出
    grpc_server->Wait();
}
```

**关键知识点**：
- 指数退避重试（第 107-134 行）：应对启动时下游服务未就绪
- 双监听模式：WebSocket(9090) + gRPC(9091)
- 心跳机制：防止僵尸连接

### 5.2 ZoneSvr 的 SystemManager

**文件**：`backend/zonesvr/internal/system/system_manager.cpp`

```cpp
// SystemManager 初始化流程
bool SystemManager::Init(const ZoneConfig& config) {
    // 1. 创建 SessionStore（存储在线用户）
    session_store_ = std::make_shared<SessionStore>();
    
    // 2. 创建各 System 实例
    auth_system_ = std::make_unique<AuthSystem>();
    chat_system_ = std::make_unique<ChatSystem>();
    friend_system_ = std::make_unique<FriendSystem>();
    group_system_ = std::make_unique<GroupSystem>();
    file_system_ = std::make_unique<FileSystem>();
    
    // 3. 初始化各 System 的 RPC 客户端
    auth_system_->Init(config.auth_svr_addr);
    chat_system_->Init(config.chat_svr_addr);
    friend_system_->Init(config.friend_svr_addr);
    // ...
    
    return true;
}

// 请求分发示例
void HandleClientRequest(const std::string& cmd, ...) {
    if (cmd == "auth.login") {
        auth_system_->Login(request);
    } else if (cmd == "chat.send_message") {
        chat_system_->SendMessage(request);
    } else if (cmd == "friend.add") {
        friend_system_->AddFriend(request);
    }
    // ...
}
```

### 5.3 AsyncLogger 异步日志

**文件**：`component/asynclogger/include/async_logger/async_logger.h`

**核心设计**：双缓冲 + 无锁环形队列

```cpp
// 使用示例
#include <async_logger/async_logger.h>

int main() {
    // 1. 初始化（配置文件或代码设置）
    asynclog::LogConfig config;
    config.log_dir = "./logs";
    config.max_file_size = 100 * 1024 * 1024;  // 100MB
    config.enable_console = true;
    asynclog::Init(config);
    
    // 2. 写日志（线程安全，无阻塞）
    LogInfo("Server started on port " << 9090);
    LogDebug(TAG("user", "alice"), "User logged in");
    LogError("Database connection failed: " << error_msg);
    
    // 3. 程序退出前刷新缓冲区
    asynclog::Shutdown();
}
```

**性能优势**：
- 业务线程：无锁写入环形缓冲（纳秒级）
- 后台线程：批量刷盘（减少 I/O 次数）
- 双缓冲交换：避免频繁分配内存

### 5.4 消息存储设计

**文件**：`backend/chatsvr/internal/store/message_store.cpp`

**RocksDB Key 规范**：

```cpp
// 消息存储 Key 设计
// 格式：msg:{msg_id}
std::string key = "msg:" + msg_id;
std::string value = message_data.SerializeAsString();
db_->Put(rocksdb::WriteOptions(), key, value);

// 会话时间线索引（用于拉取历史）
// 格式：chat:{conversation_id}:{reverse_timestamp}:{msg_id}
std::string timeline_key = "chat:" + conv_id + ":" 
                           + std::to_string(INT64_MAX - timestamp) + ":" 
                           + msg_id;
db_->Put(rocksdb::WriteOptions(), timeline_key, msg_id);

// 离线消息队列
// 格式：offline:{user_id}:{reverse_timestamp}:{msg_id}
std::string offline_key = "offline:" + user_id + ":" 
                          + std::to_string(INT64_MAX - timestamp) + ":" 
                          + msg_id;
db_->Put(rocksdb::WriteOptions(), offline_key, msg_id);
```

**拉取历史消息**：

```cpp
std::vector<MessageData> GetHistory(
    const std::string& conversation_id,
    int chat_type,
    const std::string& before_msg_id,
    int limit) {
    
    // 1. 构造前缀 key
    std::string prefix = "chat:" + conversation_id + ":";
    
    // 2. 使用 RocksDB 迭代器倒序遍历（最新在前）
    auto it = db_->NewIterator(rocksdb::ReadOptions());
    it->Seek(prefix);
    
    std::vector<MessageData> messages;
    for (; it->Valid() && messages.size() < limit; it->Next()) {
        std::string msg_id = ExtractMsgId(it->key());
        messages.push_back(GetMessageById(msg_id));
    }
    
    delete it;
    return messages;
}
```

---

## 6. 实践任务

### Task 1：编译与运行（难度：⭐）

**目标**：成功编译并启动所有服务

```bash
# 检查清单
□ 安装所有依赖
□ 编译无错误
□ 7 个服务全部启动
□ 端口监听正常
□ 查看日志无 ERROR
```

**验证方法**：
```bash
# 检查进程
ps aux | grep -E 'authsvr|onlinesvr|friendsvr|chatsvr|filesvr|zonesvr|gatesvr'

# 检查端口
netstat -tlnp | grep -E '909[0-2]|909[4-6]|9098|9100|8080'

# 查看日志
tail -f logs/gatesvr_*.log
```

### Task 2：理解认证流程（难度：⭐⭐）

**目标**：手动模拟一次登录流程

```bash
# 1. 注册用户
# （需要编写简单的 gRPC 客户端或使用 postman）
curl -X POST http://localhost:9094/auth.Register \
  -d '{"username":"test","password":"123456"}'

# 2. 验证凭证
curl -X POST http://localhost:9094/auth.VerifyCredentials \
  -d '{"username":"test","password":"123456"}'
# 返回 user_id

# 3. 登录获取 token
curl -X POST http://localhost:9095/online.Login \
  -d '{"user_id":"u_xxx","device_id":"pc-1"}'
# 返回 token

# 4. 验证 token
curl -X POST http://localhost:9095/online.ValidateToken \
  -d '{"token":"eyJhbG..."}'
```

**思考题**：
1. Token 的有效期是多久？在哪里配置？
2. 如果用户在 A 设备登录后，又在 B 设备登录，会发生什么？
3. Logout 后 Token 立即失效吗？为什么？

### Task 3：添加新日志级别（难度：⭐⭐⭐）

**目标**：在 AsyncLogger 中添加 `AUDIT` 级别（用于审计日志）

**步骤**：
1. 在 `log_level.h` 中添加 `AUDIT = 7`
2. 在 `log_config.h` 中配置独立输出文件
3. 添加宏 `LogAudit(...)` 
4. 在 AuthSvr 的注册接口中使用

**代码位置**：
- `component/asynclogger/include/async_logger/log_level.h`
- `component/asynclogger/include/async_logger/log_config.h`

### Task 4：实现简单统计功能（难度：⭐⭐⭐⭐）

**目标**：统计每个用户每天发送的消息数

**方案**：
1. 在 ChatSvr 的 `SaveMessage` 中添加计数逻辑
2. 使用 RocksDB 的 Counter Key：`stat:daily:{user_id}:{date}`
3. 提供 `GetDailyStats(user_id, date)` 接口

**扩展**：
- 每小时消息峰值统计
- 群聊活跃度排名
- 用户在线时长统计

### Task 5：Docker 容器化（难度：⭐⭐⭐⭐⭐）

**目标**：将所有服务打包成 Docker 镜像

**步骤**：
1. 阅读 `backend/*/Dockerfile`（已有示例）
2. 修改 `deploy/docker-compose.yml`
3. 构建镜像：`./deploy/docker/build-all.sh`
4. 启动：`docker compose -f deploy/docker-compose.yml up -d`

**验证**：
```bash
docker compose ps
docker compose logs -f gatesvr
```

---

## 7. 常见问题

### Q1: 编译时报错找不到 xxx.h

**原因**：vcpkg 依赖未正确安装或路径不对

**解决**：
```bash
# 1. 检查 vcpkg 是否正确安装
echo $VCPKG_ROOT

# 2. 清理 build 目录重新编译
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
make -j$(nproc)

# 3. 如果仍报错，手动安装缺失的包
$VCPKG_ROOT/vcpkg install boost-beast grpc rocksdb
```

### Q2: 服务启动后立即退出

**原因**：配置文件错误或端口被占用

**排查步骤**：
```bash
# 1. 查看日志
cat logs/*.log | tail -100

# 2. 检查端口占用
lsof -i :9090
lsof -i :9091
# ...

# 3. 检查配置文件语法
cat config/gatesvr.conf
# 确保 key=value 格式，无多余空格
```

### Q3: GateSvr 一直报 "ZoneSvr not ready"

**原因**：ZoneSvr 启动失败或地址配置错误

**解决**：
```bash
# 1. 检查 zonesvr.conf 中的监听地址
cat config/zonesvr.conf | grep grpc_port

# 2. 检查 gatesvr.conf 中的 zone_svr_addr
cat config/gatesvr.conf | grep zone_svr_addr
# 应该与 zonesvr 的实际地址一致（如 localhost:9092）

# 3. 手动测试连通性
telnet localhost 9092
# 如果不通，说明 ZoneSvr 未正常启动
```

### Q4: JWT Token 验证失败

**可能原因**：
1. jwt_secret 配置不一致
2. Token 已过期
3. 时钟不同步

**排查**：
```bash
# 1. 检查所有服务的 jwt_secret 是否一致
grep jwt_secret config/*.conf

# 2. 检查 Token 过期时间
# JWT payload 中包含 exp 字段，可用 jwt.io 解码查看

# 3. 同步系统时间
sudo ntpdate pool.ntp.org
```

### Q5: 消息发送成功但对方收不到

**排查思路**：
```bash
# 1. 检查接收方是否在线
# 查看 ZoneSvr 的 SessionStore

# 2. 检查消息是否正确存储
# 查看 ChatSvr 日志

# 3. 检查路由是否正确
# ZoneSvr 查询 session:{receiver_id} 得到正确的 Gate 地址

# 4. 检查 Gate 推送是否成功
# 查看接收方连接的 GateSvr 日志
```

---

## 附录 A：学习资源推荐

### C++ 基础
- 📖 《C++ Primer Plus》
- 🌐 https://learn.microsoft.com/zh-cn/cpp/

### 网络编程
- 📖 《Linux 高性能服务器编程》
- 🌐 Boost.Beast 官方文档

### gRPC
- 🌐 https://grpc.io/docs/languages/cpp/
- 📺 B 站：gRPC 从入门到实战

### RocksDB
- 🌐 https://rocksdb.org/docs/getting-started.html

### Kubernetes
- 📖 《Kubernetes 权威指南》
- 🌐 https://kubernetes.io/zh-cn/docs/tutorials/

---

## 附录 B：调试技巧

### GDB 调试示例

```bash
# 1. 编译 Debug 版本
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 2. 启动 gdb
gdb ./backend/gatesvr/gatesvr

# 3. 设置断点
(gdb) break GateService::RegisterGate
(gdb) break WebSocketHandler::OnMessage

# 4. 运行
(gdb) run ../config/gatesvr.conf

# 5. 查看堆栈
(gdb) bt
```

### 性能分析

```bash
# 使用 perf 分析 CPU 热点
perf record -F 99 -p $(pgrep gatesvr) sleep 10
perf report

# 查看内存使用
valgrind --leak-check=full ./backend/gatesvr/gatesvr ../config/gatesvr.conf
```

---

## 附录 C：贡献指南

### 代码规范

```cpp
// 1. 命名规范
class UserService {};           // 类名：大驼峰
void send_message() {}          // 函数名：小写 + 下划线
std::string user_id;            // 变量名：小写 + 下划线
constexpr int kMaxRetries = 3;  // 常量：k+ 大驼峰

// 2. 注释风格
/**
 * @brief 函数简要说明
 * @param param1 参数说明
 * @return 返回值说明
 */

// 3. 日志使用
LogInfo("Operation completed: " << result);
LogError(TAG("module", "auth"), "Failed: " << error);
```

### 提交流程

```bash
# 1. 创建功能分支
git checkout -b feature/add-emoji

# 2. 开发并测试
# ...

# 3. 提交代码
git add .
git commit -m "feat: add emoji support in chat"

# 4. 推送到远程
git push origin feature/add-emoji

# 5. 创建 Pull Request
```

---

## 结语

恭喜你完成了这份学习指南！🎉

**下一步建议**：
1. 加入项目讨论群（如果有）
2. 关注项目的 GitHub Issues
3. 尝试修复一个简单的 bug
4. 向朋友讲解这个项目（费曼学习法）

**记住**：
- 遇到错误不要怕，这是学习的机会
- 多问为什么，深入理解设计思想
- 动手实践胜过纸上谈兵

祝你学习愉快！🚀

---

**文档版本**：v1.0  
**最后更新**：2026-03-16  
**维护者**：Lingma AI Assistant
