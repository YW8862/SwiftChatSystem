# 高性能可扩展社交平台 —— 完整开发设计文档

C++ 微服务架构 · gRPC + Protobuf · Minikube 部署 · Windows 客户端

---

## 1. 项目概述

### 1.1 目标

构建一个功能完整、高性能、可演示的类 QQ 实时社交系统，支持：

- 私聊 / 群聊 / 好友关系 / 富媒体消息
- 消息撤回 / @提醒 / 已读回执 / 离线推送 / 消息搜索
- Windows 客户端（.exe 安装包）
- 独立可复用的异步日志系统（AsyncLogger）
- Minikube 本地 Kubernetes 部署

### 1.2 核心价值

| 维度     | 说明                                   |
| -------- | -------------------------------------- |
| 产品感   | 功能闭环，可现场安装演示               |
| 技术深度 | C++ 系统编程 + 微服务 + 云原生         |
| 工程规范 | gRPC IDL + CMake + Kubernetes          |
| 可复用性 | 异步日志系统可独立使用于其他 C++ 项目  |

---

## 2. 系统架构

### 2.1 整体架构图

```
                        ┌─────────────────┐
                        │  LoadBalancer   │
                        └────────┬────────┘
                                 │
         ┌───────────────────────┼───────────────────────┐
         ▼                       ▼                       ▼
    ┌─────────┐             ┌─────────┐             ┌─────────┐
    │ GateSvr │             │ GateSvr │             │ GateSvr │
    │  (Pod)  │             │  (Pod)  │             │  (Pod)  │
    └────┬────┘             └────┬────┘             └────┬────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 ▼
                        ┌─────────────────┐
                        │    ZoneSvr      │ ←── 路由层（会话状态）
                        └────────┬────────┘
                                 │
          ┌──────────────────────┼──────────────────────┬──────────────────────┐
          ▼                      ▼                      ▼                      ▼
    ┌──────────┐          ┌──────────┐          ┌──────────┐          ┌──────────┐
    │ AuthSvr  │          │OnlineSvr │          │ ChatSvr  │          │ FileSvr  │
    └────┬─────┘          └────┬─────┘          └────┬─────┘          └────┬─────┘
         │                     │                     │                     │
    ┌────▼─────┐          ┌────▼─────┐          ┌────▼─────┐          ┌────▼─────┐
    │  Store   │          │  Store   │          │  Store   │          │  Store   │
    └──────────┘          └──────────┘          └──────────┘          └──────────┘
```

### 2.2 ZoneSvr-System 架构（核心设计）

采用 **Zone-System 模式**，ZoneSvr 作为中心服务，内部包含多个子系统：

```
                         ┌─────────────────────────────────────────────┐
                         │                  ZoneSvr                    │
                         │  ┌─────────────────────────────────────┐    │
                         │  │          SystemManager              │    │
                         │  │  ┌───────────┐  ┌───────────┐       │    │
 GateSvr ──gRPC──────────│──│  │AuthSystem │  │ChatSystem │       │    │
                         │  │  └─────┬─────┘  └─────┬─────┘       │    │
                         │  │        │              │             │    │
                         │  │  ┌─────┴─────┐  ┌─────┴─────┐       │    │
                         │  │  │FriendSys  │  │GroupSystem│       │    │
                         │  │  └─────┬─────┘  └─────┬─────┘       │    │
                         │  │        │              │             │    │
                         │  │  ┌─────┴─────────────┴─────┐        │    │
                         │  │  │       FileSystem        │        │    │
                         │  │  └─────────────────────────┘        │    │
                         │  └─────────────────────────────────────┘    │
                         │                    │                        │
                         │           ┌────────▼────────┐               │
                         │           │  SessionStore   │               │
                         │           └─────────────────┘               │
                         └─────────────────────────────────────────────┘
                                              │
              ┌───────────────────────────────┼───────────────────────────────┐
              ▼                               ▼                               ▼
         AuthSvr(RPC)                   ChatSvr(RPC)                    FileSvr(RPC)
              │                               │                               │
         ┌────▼────┐                     ┌────▼────┐                     ┌────▼────┐
         │  Store  │                     │  Store  │                     │  Store  │
         └─────────┘                     └─────────┘                     └─────────┘
```

**核心组件说明：**

| 组件 | 职责 |
|------|------|
| **SystemManager** | 管理所有 System 的生命周期，分发请求 |
| **AuthSystem** | 登录/登出走 OnlineSvr，身份与资料走 AuthSvr（VerifyCredentials + OnlineSvr.Login） |
| **ChatSystem** | RPC 转发层，调用 ChatSvr，协调消息路由 |
| **FriendSystem** | RPC 转发层，调用 FriendSvr |
| **GroupSystem** | RPC 转发层，调用 ChatSvr(GroupService)；创建群至少 3 人，邀请仅对非成员生效 |
| **FileSystem** | RPC 转发层，调用 FileSvr |
| **SessionStore** | ZoneSvr 内在线会话（消息路由）；登录会话在 OnlineSvr |

**架构模式：API Gateway + 微服务**
  find . -name '.*' -prune -o -type f -print | wc -l
```
┌────────────────────────────────────────────────────────────────────┐
│                      ZoneSvr (API Gateway)                         │
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                    RPC Clients (统一持有)                     │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐ │  │
│  │  │AuthClient  │ │OnlineClient│ │ChatClient  │ │FriendClient│ │ FileClient │ │  │
│  │  └─────┬──────┘ └─────┬──────┘ └─────┬──────┘ └─────┬──────┘ └─────┬──────┘ │  │
│  └────────┼──────────────┼──────────────┼──────────────┼──────────────┘  │
│           │              │              │              │              │
│  ┌────────┼──────────────┼──────────────┼──────────────┼──────────────┐  │
│  │        │         SessionStore (消息路由)                 │        │  │
│  └────────┼──────────────┼──────────────┼──────────────┼──────────────┘  │
└───────────┼──────────────┼──────────────┼──────────────┼──────────────┘
            │              │              │              │              │
            ▼              ▼              ▼              ▼              ▼
       ┌─────────┐ ┌─────────┐ ┌─────────┐    ┌─────────┐    ┌─────────┐
       │ AuthSvr │ │OnlineSvr│ │ ChatSvr │───→│ FileSvr │    │FriendSvr│
       │(身份/资料)│ │(登录会话)│ │(实际逻辑)│    │(实际逻辑)│   │(实际逻辑)│
       └─────────┘ └─────────┘ └─────────┘    └─────────┘    └─────────┘
                           │              ↑
                           └──────────────┘
                          (服务间直接调用)
```

**职责划分：**

| 组件 | 职责 |
|------|------|
| **ZoneSvr** | 统一入口、持有所有 RPC Client、请求转发、消息路由 |
| **各 System** | 封装对应服务的 RPC 调用接口 |
| **SessionStore** | 用户会话状态，用于消息路由到正确的 Gate |
| **后端 Svr** | 实际业务逻辑、数据存储 |
| **服务间调用** | 后端服务自己持有其他服务的 Client（如 ChatSvr → FileSvr） |

**请求流程：**

```
1. Client → GateSvr (WebSocket)
2. GateSvr → ZoneSvr (gRPC)
3. ZoneSvr 根据 cmd 选择对应的 System/Client
4. System 通过 RPC Client 转发请求到后端 Svr
5. 后端 Svr 处理业务逻辑，返回结果
6. ZoneSvr 返回给 GateSvr → Client
```

**设计优势：**

- **统一入口**：所有请求经过 ZoneSvr，便于鉴权、限流、监控
- **RPC Client 集中管理**：连接池、超时、重试策略统一配置
- **会话共享**：SessionStore 用于消息路由，无需每次查 Redis
- **职责清晰**：ZoneSvr 只转发，业务逻辑在后端服务
- **服务解耦**：后端服务间调用自己管理，互不影响

### 2.3 后端服务分层（无独立 API 层）

各后端服务（AuthSvr、ChatSvr 等）采用三层结构，**对外 API 由 Handler 层直接实现**，不设单独的 API 层：

```
┌─────────────────────────────────────────────────────────┐
│  Handler 层（对外 API）                                  │
│  - 实现 proto 定义的 gRPC 接口                            │
│  - 解析请求、组装响应，调用 Service                       │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│  Service 层（业务逻辑）                                  │
│  - 注册/登录、消息收发、好友关系等                        │
│  - 调用 Store 读写数据                                   │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│  Store 层（数据持久化）                                  │
│  - RocksDB/MySQL/Redis 读写                              │
└─────────────────────────────────────────────────────────┘
```

| 层级 | 职责 | 说明 |
|------|------|------|
| **Handler** | 对外 API | 直接实现 gRPC Service，无单独 API 层 |
| **Service** | 业务逻辑 | 核心业务处理 |
| **Store** | 数据持久化 | 存储抽象与实现 |

- **OnlineSvr** 的 Service 层依赖 `SessionStore`（RocksDB）记录登录会话，使用公共库 `swift/jwt_helper.h` 签发/校验 JWT，单设备在线策略在此实现；登录/登出/验 Token 统一走 OnlineSvr。**AuthSvr** 仅负责注册、VerifyCredentials（校验用户名密码并返回 user_id、profile）、GetProfile、UpdateProfile，无 SessionStore，无 Login/Logout/ValidateToken。

- **业务接口鉴权（防伪造 user_id）**：凡涉及「当前用户」的接口（如 GetProfile、UpdateProfile、好友列表、发消息等），客户端须在 gRPC metadata 中携带 JWT（`authorization: Bearer <token>` 或 `x-token: <token>`）。AuthSvr、FriendSvr、ChatSvr 的 Handler 使用公共库 `swift_grpc_auth` 从 metadata 校验 Token 得到 `user_id`，**仅以该身份执行业务，不再信任请求体中的 user_id**，从而避免越权。Register、VerifyCredentials 为登录前接口，不需 Token。详见 develop.md 第 16 节。

### 2.4 统一会话模型（私聊与群聊）

- **抽象**：私聊与群聊统一为「会话」；区别仅为可见范围与是否具备群专属能力。
- **私聊**：视为**仅两人的会话**（type=private）。两好友之间**仅存在一个**私聊会话，通过 `GetOrCreatePrivateConversation(u1, u2)` 得到唯一 `conversation_id`（如 `p_<min>_<max>`）。
- **群聊**：多人会话（type=group），`conversation_id` 即 `group_id`。**同一批人（≥3）可建多个群**（如「家人群」「家人-旅游群」）。
- **消息**：统一按 `conversation_id` 存储与拉取历史，不区分私聊/群聊。
- **群专属能力**：拉人进群、踢人、转让群主、设置管理员、群公告等**仅当 type=group 时允许**，接口内校验 `conversation.type == group`，否则返回错误（如 NOT_GROUP_CHAT）。

### 2.5 关键设计决策

| 决策点           | 方案                                      | 理由                       |
| ---------------- | ----------------------------------------- | -------------------------- |
| 私聊/群聊模型    | 统一会话（私聊=两人会话，群聊=多人会话） | 消息与历史逻辑统一，群能力按 type 区分 |
| 架构模式         | Zone-System 模式                          | 统一入口、状态共享、便于维护 |
| 对外 API         | Handler 层直接实现                        | 无独立 API 层，Handler 即 gRPC 接口 |
| 客户端协议       | WebSocket + Protobuf（二进制）            | 体积小、解析快、与后端统一 |
| 会话管理         | ZoneSvr + Redis                           | 支持多副本、高可用         |
| 存储引擎         | RocksDB（单机）/ MySQL（集群）            | 灵活切换、渐进式扩展       |
| 日志方案         | 异步日志库（进程内）                      | 高性能、可复用、无网络开销 |
| 服务合并         | ChatSvr 含离线消息 + 搜索                 | 减少跨服务调用和数据同步   |

### 2.6 配置管理

- **方式**：各服务通过 **配置文件（.conf）+ 环境变量覆盖** 加载配置，配置项不写死在代码中，便于多环境与容器部署。
- **格式**：key=value，支持 `#` 注释；键名不区分大小写。示例与说明见项目根目录 `config/` 下的 `*.conf.example` 及 `config/README.md`。
- **公共实现**：`backend/common` 提供 `swift::KeyValueConfig`（`config_loader.h`），统一实现「读 key=value 文件」与「按前缀应用环境变量覆盖」（如 `FILESVR_GRPC_PORT` → `grpc_port`）。各服务 `internal/config/config.cpp` 仅调用 `LoadKeyValueConfig(path, "PREFIX_")` 并将结果填到本服务的配置结构体。
- **约定**：配置文件路径可由命令行参数或环境变量 `XXX_CONFIG` 指定；敏感项（如 `jwt_secret`）建议仅通过环境变量注入，不写入仓库内的 .conf。

---

## 3. 微服务列表与职责

| 服务          | 协议                    | 存储                | 关键能力                           |
| ------------- | ----------------------- | ------------------- | ---------------------------------- |
| GateSvr       | WebSocket + gRPC        | 内存                | 连接管理、协议解析、消息转发       |
| ZoneSvr       | gRPC                    | Redis/内存          | 在线状态、路由广播、Gate 管理      |
| AuthSvr       | gRPC                    | RocksDB             | 注册、VerifyCredentials、GetProfile、UpdateProfile（身份与资料） |
| **OnlineSvr** | gRPC                    | RocksDB             | **登录会话、Login/Logout/ValidateToken、JWT 签发、单设备策略** |
| FriendSvr     | gRPC                    | RocksDB/MySQL       | 好友/分组/黑名单                   |
| ChatSvr       | gRPC                    | RocksDB/MySQL       | 消息收发/撤回/@/离线队列/搜索      |
| FileSvr       | gRPC + HTTP             | RocksDB + 本地/MinIO | 文件上传/下载                      |

**服务数量：7 个**（含 OnlineSvr）

---

## 4. 存储架构设计

### 4.1 分层存储架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        存储抽象层                                │
├─────────────────────────────────────────────────────────────────┤
│   Store 接口（UserStore / MessageStore / SessionStore ...）      │
├──────────────────┬──────────────────┬───────────────────────────┤
│   RocksDB 实现   │   MySQL 实现      │   Redis 实现              │
│   (单机/开发)    │   (生产/多副本)   │   (会话/缓存)             │
└──────────────────┴──────────────────┴───────────────────────────┘
```

### 4.2 各服务存储方案

| 服务 | 数据类型 | 单机方案 | 集群方案 | Key 设计 |
|------|---------|---------|---------|----------|
| **AuthSvr** | 用户信息 | RocksDB | MySQL | `user:{id}`, `username:{name}→id` |
| **OnlineSvr** | 登录会话 | RocksDB | Redis（可选扩展） | `session:{user_id}`（ZoneSvr 登录/登出走此服务） |
| **FriendSvr** | 好友关系 | RocksDB | MySQL | `friend:{uid}:{fid}`, `block:{uid}:{tid}` |
| **ChatSvr** | 消息数据 | RocksDB | MySQL + 分表 | `msg:{id}`, `chat:{conversation_id}:{rev_ts}:{mid}` |
| **ChatSvr** | 会话元信息（私聊） | RocksDB | - | `conv_meta:{conversation_id}`（私聊 get-or-create） |
| **ChatSvr** | 离线消息 | RocksDB | Redis List | `offline:{uid}:{rev_ts}:{mid}` |
| **FileSvr** | 文件元信息 | RocksDB | MySQL | `file:{id}`, `md5:{hash}→id` |
| **FileSvr** | 文件内容 | 本地磁盘 | MinIO/S3 | `/files/{date}/{file_id}` |
| **ZoneSvr** | 在线状态 | 内存 | Redis | `session:{uid}`, `gate:{gid}` |

### 4.3 RocksDB Key 设计规范

```
格式：{类型}:{主键}:{可选子键}

示例：
  user:u_123456                     → 用户数据 JSON
  username:alice                    → u_123456
  friend:u_123:u_456                → 好友关系数据
  msg:m_789                         → 消息数据 JSON
  chat:c_123:rev_ts:m_789           → 会话时间线（conversation_id=私聊/群聊统一 id）
  conv_meta:p_u1_u2                 → 私聊会话元信息（type=private）
  offline:u_123:rev_ts:m_789        → 离线消息索引
```

### 4.4 Store 接口设计

```cpp
// 用户存储接口
class UserStore {
public:
    virtual bool Create(const UserData& user) = 0;
    virtual std::optional<UserData> GetById(const std::string& user_id) = 0;
    virtual std::optional<UserData> GetByUsername(const std::string& username) = 0;
    virtual bool Update(const UserData& user) = 0;
};

// 统一会话模型：私聊 = 两人会话（type=private），两好友间唯一会话；群聊 = 多人会话（type=group），同一批人可建多群。消息统一按 conversation_id 存储。

// 会话注册（私聊 get-or-create）
class ConversationRegistry {
public:
    virtual std::string GetOrCreatePrivateConversation(const std::string& user_id_1,
                                                         const std::string& user_id_2) = 0;
};

// 消息存储接口（conversation_id = 私聊会话 id 或 group_id）
class MessageStore {
public:
    virtual bool Save(const MessageData& msg) = 0;
    virtual std::optional<MessageData> GetById(const std::string& msg_id) = 0;
    virtual std::vector<MessageData> GetHistory(const std::string& conversation_id,
                                                 int chat_type,
                                                 const std::string& before_msg_id, int limit) = 0;
    virtual bool MarkRecalled(const std::string& msg_id, int64_t recall_at) = 0;
    virtual bool AddToOffline(const std::string& user_id, const std::string& msg_id) = 0;
    virtual std::vector<MessageData> PullOffline(...) = 0;
    virtual bool ClearOffline(const std::string& user_id, const std::string& until_msg_id) = 0;
};

// 会话存储接口
class SessionStore {
public:
    virtual bool SetOnline(const UserSession& session) = 0;
    virtual bool SetOffline(const std::string& user_id) = 0;
    virtual std::optional<UserSession> GetSession(const std::string& user_id) = 0;
    virtual bool IsOnline(const std::string& user_id) = 0;
};
```

---

## 5. Redis 设计（集群模式）

### 5.1 用途划分

| 用途 | Key 前缀 | 数据结构 | TTL |
|------|---------|---------|-----|
| 用户会话 | `session:` | Hash | 1h（心跳续期） |
| Gate 节点 | `gate:` | Hash | 60s（心跳续期） |
| Gate 列表 | `gate:list` | Set | - |
| 离线消息队列 | `offline:` | List | 7d |
| 消息已读位置 | `read:` | String | - |
| 用户 Token | `token:` | String | 7d |

### 5.2 Key 结构设计

```
# 用户会话
session:{user_id} = {
  gate_id: "gate-1",
  gate_addr: "10.0.0.1:9091",
  device_type: "windows",
  online_at: 1706800000,
  last_active: 1706800100
}
EXPIRE session:{user_id} 3600

# Gate 节点
gate:{gate_id} = {
  address: "10.0.0.1:9091",
  connections: 1500,
  registered_at: 1706800000
}
EXPIRE gate:{gate_id} 60

# Gate 列表
gate:list = [gate-1, gate-2, gate-3]

# 离线消息
offline:{user_id} = [msg_id_1, msg_id_2, ...]
```

---

## 6. 集群路由设计

### 6.1 路由架构

```
                     ┌─────────────────┐
                     │  LoadBalancer   │
                     └────────┬────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
   ┌─────────┐           ┌─────────┐           ┌─────────┐
   │ Gate-1  │           │ Gate-2  │           │ Gate-3  │
   └────┬────┘           └────┬────┘           └────┬────┘
        │                     │                     │
        └─────────────────────┼─────────────────────┘
                              ▼
                     ┌─────────────────┐
                     │    ZoneSvr      │ ←── 查询 Redis
                     └────────┬────────┘
                              │
                     ┌────────▼────────┐
                     │     Redis       │ ←── 共享会话状态
                     └─────────────────┘
```

### 6.2 消息路由流程

```
Alice (Gate-1) 发消息给 Bob (Gate-2)

1. Gate-1 收到 Alice 的消息
2. Gate-1 → ZoneSvr.RouteMessage(to_user=bob, payload=...)
3. ZoneSvr 查询 Redis: HGETALL session:bob
   → 获取 bob 在 Gate-2，地址 10.0.0.2:9091
4. ZoneSvr → Gate-2.PushMessage(user_id=bob, payload=...)
5. Gate-2 通过 WebSocket 推送给 Bob
6. 如果 Bob 离线：
   - ChatSvr 存储消息
   - LPUSH offline:bob msg_id
```

### 6.3 Gate 注册与心跳

```
Gate 启动：
  1. 生成唯一 gate_id
  2. 调用 ZoneSvr.GateRegister(gate_id, address)
  3. ZoneSvr 写入 Redis: HSET gate:{gate_id} ..., SADD gate:list {gate_id}

Gate 心跳（每 30s）：
  1. 调用 ZoneSvr.GateHeartbeat(gate_id, connections)
  2. ZoneSvr 更新 Redis: EXPIRE gate:{gate_id} 60

用户上线：
  1. Gate 调用 ZoneSvr.UserOnline(user_id, gate_id, device_type)
  2. ZoneSvr 写入 Redis: HSET session:{user_id} ...
```

---

## 7. 服务发现与负载均衡

### 7.1 方案选择

| 方案 | 复杂度 | 适用场景 |
|------|--------|----------|
| **K8s ClusterIP Service** | 低 | 开发/小规模，L4 轮询 |
| **gRPC 客户端负载均衡** | 中 | 生产推荐，配合 Headless Service |
| **Envoy Sidecar** | 高 | 大规模/服务网格 (Istio) |

### 7.2 推荐方案：gRPC 客户端负载均衡

```
┌─────────────────────────────────────────────────────────────────┐
│                        Kubernetes Cluster                        │
│                                                                 │
│  ┌─────────────┐      Headless Service (DNS)                   │
│  │   ZoneSvr   │      authsvr.swift.svc.cluster.local          │
│  │             │          │                                     │
│  │ AuthClient  │──DNS────→├── 10.0.0.1 (authsvr-pod-1)         │
│  │ (gRPC LB)   │          ├── 10.0.0.2 (authsvr-pod-2)         │
│  │             │          └── 10.0.0.3 (authsvr-pod-3)         │
│  └─────────────┘                                                │
│        │                                                        │
│        │ gRPC 客户端负载均衡 (round_robin)                       │
│        ▼                                                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                      │
│  │ AuthSvr  │  │ AuthSvr  │  │ AuthSvr  │                      │
│  │  Pod-1   │  │  Pod-2   │  │  Pod-3   │                      │
│  └──────────┘  └──────────┘  └──────────┘                      │
└─────────────────────────────────────────────────────────────────┘
```

### 7.3 K8s Headless Service 配置

```yaml
apiVersion: v1
kind: Service
metadata:
  name: authsvr
  namespace: swift
spec:
  clusterIP: None          # Headless Service，返回所有 Pod IP
  selector:
    app: authsvr
  ports:
    - name: grpc
      port: 9094
```

### 7.4 gRPC 客户端负载均衡实现

```cpp
// 创建支持负载均衡的 Channel
std::shared_ptr<grpc::Channel> CreateLBChannel(
    const std::string& service_name,
    const std::string& ns = "swift",
    int port = 9094) {
    
    // DNS 格式：dns:///service.namespace.svc.cluster.local:port
    std::string target = "dns:///" + service_name + "." + ns + 
                         ".svc.cluster.local:" + std::to_string(port);
    
    grpc::ChannelArguments args;
    args.SetLoadBalancingPolicyName("round_robin");  // 轮询策略
    
    return grpc::CreateCustomChannel(
        target,
        grpc::InsecureChannelCredentials(),
        args
    );
}
```

### 7.5 各层负载均衡方式

| 层级 | 负载均衡方式 |
|------|-------------|
| **Client → GateSvr** | K8s Ingress / LoadBalancer |
| **GateSvr → ZoneSvr** | gRPC 客户端负载均衡 + Headless Service |
| **ZoneSvr → 各 Svr** | gRPC 客户端负载均衡 + Headless Service |
| **Svr 间调用** | 同上 |

---

## 8. 部署模式

### 7.1 模式对比

| 模式 | 适用场景 | 存储方案 | 副本数 |
|------|---------|---------|--------|
| **单机模式** | 开发/演示 | RocksDB + 内存 + 本地磁盘 | 各服务 1 |
| **集群模式** | 生产 | MySQL + Redis + MinIO | Gate 3+, 其他 2+ |

### 7.2 单机模式配置

```yaml
# 环境变量
STORE_TYPE: rocksdb
SESSION_STORE_TYPE: memory
FILE_STORAGE_TYPE: local
```

### 7.3 集群模式配置

```yaml
# 环境变量
STORE_TYPE: mysql
MYSQL_DSN: "user:pass@tcp(mysql:3306)/swift"
SESSION_STORE_TYPE: redis
REDIS_URL: "redis://redis:6379"
FILE_STORAGE_TYPE: minio
MINIO_ENDPOINT: "minio:9000"
```

### 7.4 服务端口

| 服务          | gRPC 端口 | 其他端口          |
| ------------- | --------- | ----------------- |
| GateSvr       | 9091      | WebSocket: 9090   |
| ZoneSvr       | 9092      | -                 |
| AuthSvr       | 9094      | -                 |
| OnlineSvr     | 9095      | -                 |
| FriendSvr     | 9096      | -                 |
| ChatSvr       | 9098      | -                 |
| FileSvr       | 9100      | HTTP: 8080        |

---

## 9. 异步日志系统设计（AsyncLogger）

### 8.1 设计目标

- **独立可复用**：作为独立库，可用于任何 C++ 项目
- **高性能异步**：日志写入不阻塞业务线程
- **零拷贝优化**：使用环形缓冲区 + 双缓冲交换
- **多后端支持**：文件、stdout、远程（可扩展）

### 8.2 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                     AsyncLogger 库                       │
├─────────────────────────────────────────────────────────┤
│  [业务线程]                                              │
│      │                                                   │
│      ↓ LOG_INFO("msg", args...)                         │
│  ┌─────────────┐                                        │
│  │ LogEntry    │  格式化 → 无锁写入                      │
│  │ Formatter   │                                        │
│  └──────┬──────┘                                        │
│         ↓                                                │
│  ┌─────────────────────────────┐                        │
│  │   RingBuffer (Lock-Free)    │  双缓冲：写满交换       │
│  │   [Buffer A] ←→ [Buffer B]  │                        │
│  └──────────────┬──────────────┘                        │
│                 ↓ (condition_variable 通知)              │
│  ┌─────────────────────────────┐                        │
│  │   Backend Thread (单线程)    │  批量刷盘              │
│  └──────────────┬──────────────┘                        │
│                 ↓                                        │
│  ┌─────────────────────────────────────────────┐        │
│  │ Sink 接口                                    │        │
│  │  ├── FileSink      (按大小/时间滚动)         │        │
│  │  ├── ConsoleSink   (stdout/stderr)          │        │
│  │  └── RemoteSink    (gRPC/HTTP，可选扩展)     │        │
│  └─────────────────────────────────────────────┘        │
└─────────────────────────────────────────────────────────┘
```

### 8.3 核心接口

```cpp
namespace asynclog {

enum class Level { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

struct LogConfig {
    Level min_level = Level::INFO;
    size_t buffer_size = 4 * 1024 * 1024;
    std::string log_dir = "./logs";
    std::string file_prefix = "app";
    size_t max_file_size = 100 * 1024 * 1024;
    int max_file_count = 10;
    bool enable_console = true;
};

void Init(const LogConfig& config);
void Shutdown();

#define LOG_INFO(fmt, ...)  asynclog::Log(asynclog::Level::INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) asynclog::Log(asynclog::Level::ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

}  // namespace asynclog
```

### 8.4 双缓冲模式复用于异步写

AsyncLogger 的 **RingBuffer（双缓冲）+ BackendThread** 实现（见 `component/asynclogger/src/ring_buffer.h`、`backend_thread.h`）是一套通用的「生产者入队、消费者批量处理」模型：

- **生产者**：业务线程调用 `Push(entry)` 写入前台缓冲，满或达阈值时 `Notify()` 唤醒消费者。
- **双缓冲交换**：消费者调用 `SwapAndGet(entries, timeout_ms)`，将前台与后台缓冲交换，一次性取走整批条目，减少锁竞争与拷贝。
- **消费者**：后台线程循环 SwapAndGet，对取到的条目做实际 I/O（日志写 Sink；若复用于其他场景，则为写盘/写 DB）。

**当前业务侧**：FileSvr、ChatSvr 等仍**直接同步写**（不经过异步写中间层），避免多余封装与拷贝。

**预留的异步写中间层**：`backend/common` 中的 `swift::async::IWriteExecutor`（见 `swift/async_write.h`）已定义并实现 `SyncWriteExecutor`，但**暂无业务调用**。当未来引入 `AsyncDbWriteExecutor`（队列 + 写线程池）时，可将 FileSvr/ChatSvr 的写路径改为通过该接口提交任务：强一致写用 `SubmitAndWait`，非核心写用 `Submit` fire-and-forget。详细设计与适配要点见 **develop.md [8.5 异步写设计方案](develop.md#85-异步写设计方案)**。

---

## 10. 核心功能流程

### 9.1 消息发送（私聊）

```
1. Client → GateSvr: WebSocket {cmd: "chat.send", payload: ...}
2. GateSvr → ZoneSvr: RouteMessage(to_user=bob, payload=...)
3. ZoneSvr 查询 Redis session:bob：
   ├── 在线 → ChatSvr.SendMessage() 存储 → Gate-X.PushMessage()
   └── 离线 → ChatSvr.SendMessage() 存储 → LPUSH offline:bob
4. 响应 → GateSvr → Client: 发送成功
```

### 9.2 消息撤回

```
限制：2 分钟内 + 发送者本人
操作：更新消息状态为已撤回，不删除原消息
广播：ZoneSvr 通知会话所有在线成员
```

### 9.3 离线消息

```
1. 用户上线 → Client 调用 PullOffline(cursor, limit)
2. ChatSvr 从 Redis LRANGE offline:{uid} 获取消息 ID 列表
3. 根据 ID 查询消息详情，返回客户端
4. 客户端确认后，LTRIM 清除已读消息
```

---

## 11. 项目目录结构

```
SwiftChatSystem/
├── CMakeLists.txt                    # 顶层构建配置
├── vcpkg.json                        # 依赖声明
├── README.md
├── system.md                         # 本设计文档
│
├── config/                           # 服务配置示例（key=value .conf）
│   ├── README.md                     # 配置说明与环境变量前缀
│   ├── authsvr.conf.example
│   ├── onlinesvr.conf.example
│   ├── friendsvr.conf.example
│   ├── chatsvr.conf.example
│   ├── filesvr.conf.example
│   ├── gatesvr.conf.example
│   └── zonesvr.conf.example
│
├── backend/                          # 后端服务
│   ├── CMakeLists.txt
│   ├── common/                       # 公共库
│   │   ├── CMakeLists.txt
│   │   ├── proto/
│   │   │   ├── CMakeLists.txt
│   │   │   └── common.proto
│   │   ├── include/swift/
│   │   │   ├── common.h
│   │   │   ├── config.h
│   │   │   ├── config_loader.h       # KeyValueConfig，统一 .conf + env 加载
│   │   │   ├── utils.h
│   │   │   ├── jwt_helper.h          # 公共 JWT（HS256），供 OnlineSvr 等使用
│   │   │   └── log_helper.h
│   │   └── src/
│   │       ├── utils.cpp
│   │       ├── config.cpp
│   │       └── jwt_helper.cpp
│   │
│   ├── gatesvr/                      # 接入网关
│   │   ├── CMakeLists.txt
│   │   ├── Dockerfile
│   │   ├── cmd/main.cpp
│   │   ├── proto/gate.proto
│   │   └── internal/
│   │       ├── handler/
│   │       ├── service/
│   │       └── config/
│   │
│   ├── zonesvr/                      # 路由服务（Zone-System 架构）
│   │   ├── CMakeLists.txt
│   │   ├── Dockerfile
│   │   ├── cmd/main.cpp
│   │   ├── proto/zone.proto
│   │   └── internal/
│   │       ├── system/               # 核心：子系统
│   │       │   ├── base_system.h     # System 基类
│   │       │   ├── system_manager.h/cpp
│   │       │   ├── auth_system.h/cpp
│   │       │   ├── chat_system.h/cpp
│   │       │   ├── friend_system.h/cpp
│   │       │   ├── group_system.h/cpp
│   │       │   └── file_system.h/cpp
│   │       ├── rpc/                  # RPC 客户端封装
│   │       │   ├── rpc_client_base.h/cpp
│   │       │   ├── auth_rpc_client.h/cpp
│   │       │   ├── online_rpc_client.h/cpp   # 登录/登出/ValidateToken 走 OnlineSvr
│   │       │   ├── chat_rpc_client.h/cpp
│   │       │   ├── friend_rpc_client.h/cpp
│   │       │   ├── group_rpc_client.h/cpp
│   │       │   ├── file_rpc_client.h/cpp
│   │       │   └── gate_rpc_client.h/cpp
│   │       ├── handler/
│   │       ├── service/
│   │       ├── store/                # SessionStore
│   │       └── config/
│   │
│   ├── authsvr/                      # 认证服务（身份+资料，无会话）
│   │   ├── CMakeLists.txt
│   │   ├── Dockerfile
│   │   ├── cmd/main.cpp
│   │   ├── proto/auth.proto
│   │   └── internal/
│   │       ├── handler/
│   │       ├── service/
│   │       ├── store/                # UserStore（无 SessionStore）
│   │       └── config/
│   │
│   ├── onlinesvr/                    # 登录会话服务
│   │   ├── CMakeLists.txt
│   │   ├── Dockerfile
│   │   ├── cmd/main.cpp
│   │   ├── proto/online.proto
│   │   └── internal/
│   │       ├── handler/
│   │       ├── service/              # 使用 common 的 swift::JwtCreate/JwtVerify
│   │       ├── store/                # SessionStore（RocksDB）
│   │       └── config/
│   │
│   ├── friendsvr/                    # 好友服务
│   │   ├── CMakeLists.txt
│   │   ├── Dockerfile
│   │   ├── cmd/main.cpp
│   │   ├── proto/friend.proto
│   │   └── internal/
│   │       ├── handler/
│   │       ├── service/
│   │       ├── store/                # FriendStore
│   │       └── config/
│   │
│   ├── chatsvr/                      # 消息服务
│   │   ├── CMakeLists.txt
│   │   ├── Dockerfile
│   │   ├── cmd/main.cpp
│   │   ├── proto/
│   │   │   ├── chat.proto
│   │   │   └── group.proto
│   │   └── internal/
│   │       ├── handler/
│   │       ├── service/
│   │       ├── store/                # MessageStore, ConversationStore, ConversationRegistry, GroupStore
│   │       └── config/
│   │
│   └── filesvr/                      # 文件服务
│       ├── CMakeLists.txt
│       ├── Dockerfile
│       ├── cmd/main.cpp
│       ├── proto/file.proto
│       └── internal/
│           ├── handler/
│           ├── service/
│           ├── store/                # FileStore
│           └── config/
│
├── client/                           # 客户端
│   ├── CMakeLists.txt
│   ├── proto/                        # 客户端 Proto
│   └── desktop/                      # Qt 桌面客户端
│       ├── CMakeLists.txt
│       ├── src/
│       │   ├── main.cpp
│       │   ├── ui/
│       │   ├── network/
│       │   ├── models/
│       │   └── utils/
│       └── resources/
│
├── deploy/                           # 部署配置
│   ├── k8s/
│   │   ├── kustomization.yaml
│   │   ├── namespace.yaml
│   │   ├── configmap.yaml
│   │   ├── persistent-volume.yaml
│   │   └── *-deployment.yaml
│   └── docker/
│
├── libs/                             # 第三方库（链接 AsyncLogger）
└── docs/                             # 文档
```

---

## 12. 技术栈汇总

| 类别     | 技术                                    |
| -------- | --------------------------------------- |
| 语言     | C++17                                   |
| RPC      | gRPC + Protobuf                         |
| 网络     | Boost.Beast (WebSocket)                 |
| 存储     | RocksDB（单机）/ MySQL（集群）          |
| 缓存     | Redis（会话状态、离线队列）             |
| 对象存储 | 本地磁盘（单机）/ MinIO（集群）         |
| 日志     | AsyncLogger（自研异步日志库）           |
| 构建     | CMake + vcpkg                           |
| 部署     | Minikube + Kubernetes                   |
| 客户端   | Qt 5.15 → Windows .exe (NSIS 安装包)    |

---

## 13. 升级路径

```
阶段 1：单机开发
  └── RocksDB + 内存 + 本地文件

阶段 2：引入 Redis
  └── ZoneSvr 会话存储迁移到 Redis
  └── 支持多 Gate 副本

阶段 3：引入 MySQL
  └── AuthSvr/FriendSvr/ChatSvr 迁移到 MySQL
  └── 消息表按月分表

阶段 4：引入对象存储
  └── FileSvr 迁移到 MinIO
  └── CDN 加速

阶段 5：完全云原生
  └── HPA 自动扩缩
  └── Prometheus + Grafana 监控
```

---

## 14. 未来扩展

| 扩展点       | 说明                                              |
| ------------ | ------------------------------------------------- |
| 监控系统     | Prometheus + Grafana（各服务暴露 /metrics）       |
| 日志集中化   | Fluentd → Loki（收集 stdout 日志）                |
| 告警         | Prometheus Alertmanager → 企业微信/邮件           |
| 多端         | Web / Android / iOS 客户端                        |

---

**文档版本**：v5.0（Zone-System 架构）  
**最后更新**：2026年2月3日
