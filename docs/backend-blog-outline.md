# SwiftChatSystem 后端系列博客大纲

## 系列一：架构与基础设施（3 篇）

### 第 1 篇：SwiftChatSystem 整体架构概览
**推荐标题：**《高性能 C++ 社交平台微服务架构设计》

**内容要点：**
- 项目目标与定位（类 QQ 的实时社交系统）
- 整体架构：Gate → Zone → System → 后端 gRPC 的请求链路
- Zone-System 模式：ZoneSvr 作为 API Gateway，System 只做 RPC 转发
- 7 个后端服务及其职责：GateSvr、ZoneSvr、AuthSvr、OnlineSvr、FriendSvr、ChatSvr、FileSvr
- 三层结构：Handler → Service → Store
- 技术选型：C++17、gRPC、Protobuf、RocksDB、Boost.Beast WebSocket

---

### 第 2 篇：Protobuf 与 gRPC 协议设计
**推荐标题：**《社交系统后端：Protobuf 协议统一设计实践》

**内容要点：**
- Proto 目录与引用关系（common、auth、online、chat、group、friend、file、gate、zone）
- `common.proto` 的设计（CommonResponse、分页、错误码）
- 服务间接口约定：请求/响应结构与错误码规范
- CMake 中 proto 的生成与依赖管理
- 统一的错误码与响应格式

---

### 第 3 篇：配置管理与公共库
**推荐标题：**《C++ 微服务中的配置加载与通用基础设施》

**内容要点：**
- 配置加载：`.conf` + 环境变量覆盖（如 `AUTHSVR_PORT`）
- `KeyValueConfig` 与 `LoadKeyValueConfig` 的使用方式
- JWT 实现：`JwtCreate`、`JwtVerify`（HS256、Base64Url）
- gRPC 鉴权：`GetAuthenticatedUserId`、metadata 解析 `authorization` / `x-token`
- 日志：AsyncLogger 的双缓冲异步写设计
- 工具函数：`utils.h`、ID 生成、时间戳等

---

## 系列二：网关与路由层（2 篇）

### 第 4 篇：GateSvr —— WebSocket 接入网关
**推荐标题：**《实时社交系统接入层：基于 Boost.Beast 的 WebSocket 网关实现》

**内容要点：**
- 职责：WebSocket 9090、gRPC 9091（供 ZoneSvr 调用）
- 连接管理：`OnConnect`、`OnMessage`、`OnDisconnect` 与 `SendToClient`
- 协议：客户端 cmd + payload、二进制 Protobuf
- 心跳与超时处理
- 登录流程：`auth.login` 与 Gate 将请求转发给 ZoneSvr
- GateService 与 ZoneRpcClient 的配合
- Gate 注册与心跳（`GateRegister`、`GateHeartbeat`）

---

### 第 5 篇：ZoneSvr —— 路由与 API Gateway
**推荐标题：**《Zone-System 模式：统一入口与 cmd 分发的路由服务设计》

**内容要点：**
- SystemManager：管理 Auth、Chat、Friend、Group、File 五类 System
- `HandleClientRequest`：按 cmd 分发到对应 System
- 各 System 作为 RPC 转发层：不实现业务逻辑，只调用对应后端 gRPC
- SessionStore：用户在线状态与消息路由（Memory / Redis）
- 用户上线 / 下线：`UserOnline`、`UserOffline`
- 消息路由：`RouteToUser`、`Broadcast`、`PushToUser`
- Internal Secret：`InternalSecretProcessor` 做内网调用认证

---

## 系列三：认证与会话（2 篇）

### 第 6 篇：AuthSvr —— 身份与用户资料
**推荐标题：**《认证服务设计：注册、密码校验与用户资料管理》

**内容要点：**
- 职责：注册、VerifyCredentials、GetProfile、UpdateProfile（不含登录会话）
- UserStore 接口与 RocksDBUserStore 实现
- Key 设计：`user:{id}`、`username:{name}→id`
- JWT 鉴权：业务接口用 `GetAuthenticatedUserId` 获取 user_id，防止越权
- 登录流程：AuthSvr.VerifyCredentials → OnlineSvr.Login

---

### 第 7 篇：OnlineSvr —— 登录会话与 JWT 签发
**推荐标题：**《登录态管理：JWT 签发、会话存储与单设备策略》

**内容要点：**
- 职责：Login、Logout、ValidateToken、JWT 签发
- SessionStore（RocksDB）：会话与 token 存储
- 单设备策略：同 user_id 多设备时的处理
- Token 生命周期：`JwtCreate`、写入 SessionStore、expire_at
- ValidateToken 与 JwtVerify 的区别：查库 vs 纯验签
- 两种鉴权场景：WebSocket 登录（ValidateToken）vs 业务接口（JwtVerify）

---

## 系列四：业务服务（3 篇）

### 第 8 篇：FriendSvr —— 好友关系
**推荐标题：**《好友系统实现：好友关系、分组与黑名单的 gRPC 设计》

**内容要点：**
- FriendStore 与 RocksDBFriendStore
- 好友申请、同意、拒绝、删除
- 好友分组
- 黑名单逻辑
- JWT 鉴权与 user_id 校验

---

### 第 9 篇：ChatSvr —— 消息与群聊
**推荐标题：**《统一会话模型：私聊与群聊的消息存储与离线队列》

**内容要点：**
- 统一会话：私聊（两人）、群聊（多人）、conversation_id
- MessageStore、ConversationStore、GroupStore
- 消息发送、历史拉取、撤回
- @ 提醒、已读回执、MarkRead
- 离线消息：AddToOffline、PullOffline、ClearOffline
- 群组：创建（至少 3 人）、邀请、踢人、转让群主

---

### 第 10 篇：FileSvr —— 文件服务
**推荐标题：**《富媒体消息支持：分块上传与 RocksDB 元数据存储》

**内容要点：**
- gRPC 元数据管理 + HTTP 上传下载（8080）
- UploadSession、分块上传、临时存储
- FileStore、RocksDBFileStore、md5 去重
- 本地磁盘与后续 MinIO 扩展

---

## 系列五：存储与通信（2 篇）

### 第 11 篇：RocksDB 存储设计与实践
**推荐标题：**《C++ 微服务中的 RocksDB 存储层设计》

**内容要点：**
- Store 接口抽象与实现：UserStore、SessionStore、FriendStore、MessageStore、GroupStore、FileStore
- Key 设计：`user:`、`username:`、`msg:`、`chat:`、`conv_meta:`、`offline:`、`file:`、`file_md5:` 等
- WriteBatch 原子写入
- 迭代器与前缀查询
- JSON 序列化与反序列化

---

### 第 12 篇：gRPC 客户端封装与服务间调用
**推荐标题：**《ZoneSvr 中的 gRPC Client 封装与 RPC 调用模式》

**内容要点：**
- RpcClientBase：Connect、CreateContext、超时、Token 注入
- 各 RPC Client：AuthRpcClient、OnlineRpcClient、ChatRpcClient、FriendRpcClient、FileRpcClient、GateRpcClient
- ZoneSvr 调用后端时如何传递 JWT（metadata）
- 负载均衡与 Headless Service（K8s）

---

## 系列六：安全与部署（2 篇）

### 第 13 篇：安全设计：JWT、鉴权与内网密钥
**推荐标题：**《社交系统后端安全实践：防越权、Token 校验与内网认证》

**内容要点：**
- API 鉴权：不信任 user_id，以 JWT 解析为准
- 登录与业务接口的两种校验路径
- ZoneSvr 内网密钥：`InternalSecretProcessor`、`x-internal-secret`
- 网络隔离与 K8s NetworkPolicy

---

### 第 14 篇：部署与运维
**推荐标题：**《SwiftChatSystem 部署实践：Docker Compose 与 Minikube》

**内容要点：**
- 本地部署：配置、启动顺序、端口规划
- Docker Compose：build、volumes、环境变量
- Minikube：多副本、服务发现、hostPath
- 配置与环境变量约定
- 未来扩展：MySQL、Redis、MinIO、HPA、Prometheus

---

## 推荐写作顺序

| 序号 | 建议顺序 | 难度 | 说明 |
|------|----------|------|------|
| 1 | 第 1 篇 | ⭐ | 总览，适合开篇 |
| 2 | 第 2 篇 | ⭐⭐ | 协议与接口约定 |
| 3 | 第 3 篇 | ⭐⭐ | 公共能力 |
| 4 | 第 4 篇 | ⭐⭐⭐ | 网关与 WebSocket |
| 5 | 第 5 篇 | ⭐⭐⭐ | 路由与 System |
| 6 | 第 6、7 篇 | ⭐⭐ | 认证与登录 |
| 7 | 第 8、9、10 篇 | ⭐⭐ | 各业务服务 |
| 8 | 第 11、12 篇 | ⭐⭐⭐ | 存储与 RPC |
| 9 | 第 13、14 篇 | ⭐⭐ | 安全与部署 |

建议每篇结构：
- 背景与要解决的问题
- 核心设计与实现
- 关键代码与调用链
- 实现中的取舍与注意事项
- 小结与可扩展方向
