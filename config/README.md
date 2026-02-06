# 服务配置文件

各服务使用 **key=value** 格式的 `.conf` 文件，支持 `#` 注释。环境变量可覆盖同名配置（前缀见下表）。

## 使用方式

1. **复制示例为实际配置**（推荐将 `*.conf` 加入 .gitignore，不提交敏感信息）：
   ```bash
   cp authsvr.conf.example authsvr.conf
   # 按需编辑 authsvr.conf
   ```

2. **指定配置文件路径**：
   - 命令行：`./authsvr /path/to/authsvr.conf`
   - 或环境变量：`export AUTHSVR_CONFIG=/path/to/authsvr.conf`

3. **环境变量覆盖**：例如 `export FILESVR_GRPC_PORT=9101` 会覆盖配置文件中的 `grpc_port`。

## 环境变量前缀

| 服务     | 前缀        | 示例                |
|----------|-------------|---------------------|
| AuthSvr  | AUTHSVR_    | AUTHSVR_PORT=9094   |
| OnlineSvr| ONLINESVR_  | ONLINESVR_JWT_SECRET=xxx |
| FriendSvr| FRIENDSVR_ | FRIENDSVR_ROCKSDB_PATH=/data/friend |
| ChatSvr  | CHATSVR_    | CHATSVR_PORT=9098   |
| FileSvr  | FILESVR_    | FILESVR_STORAGE_PATH=/data/files |
| GateSvr  | GATESVR_    | GATESVR_WEBSOCKET_PORT=9090 |
| ZoneSvr  | ZONESVR_    | ZONESVR_PORT=9092   |

## 示例文件说明

- `authsvr.conf.example` - 认证服务（注册、VerifyCredentials、资料）
- `onlinesvr.conf.example` - 登录会话（Login/Logout/ValidateToken、JWT）
- `friendsvr.conf.example` - 好友服务
- `chatsvr.conf.example` - 消息与群组服务
- `filesvr.conf.example` - 文件服务（上传/下载）
- `gatesvr.conf.example` - 接入网关（WebSocket）
- `zonesvr.conf.example` - 路由服务（API Gateway）

配置加载由公共库 `swift::KeyValueConfig`（`backend/common`）统一实现，各服务 `config.cpp` 仅做「键 → 结构体」映射。
