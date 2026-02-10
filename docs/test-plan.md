## SwiftChatSystem 测试方案（含脚本与指标）

### 1. 文档概述

- **目标**：对 SwiftChatSystem 进行系统级验证，覆盖基础功能验收、安全测试及压力/性能测试，给出可直接执行的测试脚本示例与关键指标。
- **范围**：
  - 后端服务：`GateSvr`、`ZoneSvr`、`AuthSvr`、`OnlineSvr`、`FriendSvr`、`ChatSvr`、`FileSvr`
  - 客户端：Windows 桌面客户端（Qt）
  - 部署形态：单机模式 + Minikube 集群模式

---

### 2. 基础功能测试（验收功能）

#### 2.1 用例总览

> 建议后续将本节同步为 Excel/表格维护，当前文档给出核心用例摘要与脚本思路。

- **F-001 注册与登录（正常流程）**
  - 前置：所有服务启动；客户端可连 Gate WebSocket。
  - 步骤：注册用户 → 使用账号密码登录 → WebSocket `auth.login` 绑定连接。
  - 预期：返回 token 和过期时间；用户在线可见。

- **F-002 登录失败场景**
  - 场景：错误密码 / 不存在用户。
  - 预期：返回明确错误；不泄露用户存在性细节。

- **F-003 单设备策略**
  - 步骤：同一账号在两台设备上同时登录。
  - 预期：只保留一个有效会话（具体行为以实现为准：挤下线或拒绝第二次登录）。

- **F-010 添加 / 删除 / 拉黑好友**
  - 步骤：A/B 互加好友 → 删除/拉黑 → 再次添加。
  - 预期：FriendSvr 关系/黑名单正确；被拉黑人无法继续发起会话。

- **F-020 私聊发送与接收（含离线消息）**
  - 步骤：A/B 在线互发消息；B 离线时 A 发送多条消息；B 再上线拉取离线。
  - 预期：在线即时送达；离线期间消息完整拉取。

- **F-022 群聊与成员管理**
  - 步骤：创建群 G（≥3 人）→ 发群消息 → 拉人/踢人。
  - 预期：权限控制正确；被踢用户不再收到/发送群消息。

- **F-030 高级聊天能力**
  - @提醒：mentions 字段正确；被 @ 用户收到高亮。
  - 已读回执：`chat.mark_read` 后，发送方收到 read_receipt。
  - 撤回：限制时间内可撤回，超时失败；消息状态为撤回而非删除。

- **F-040 文件收发**
  - 小/大文件上传下载；校验 MD5。
  - 预期：FileSvr 元数据与文件内容一致；下载可用。

#### 2.2 基础功能测试脚本示例

> 下列脚本为思路示例，可按需补充为完整自动化测试用例（gRPC 客户端/集成测试）。

```bash
# scripts/test_auth_flow.sh
# 基础认证流程：注册 + 登录 + token 校验

set -euo pipefail

AUTH_ADDR="${AUTH_ADDR:-localhost:9094}"
ONLINE_ADDR="${ONLINE_ADDR:-localhost:9095}"

USERNAME="test_user_$RANDOM"
PASSWORD="Password123!"

echo "[1] 注册用户 $USERNAME"
# TODO: 使用自编写的 gRPC CLI / 集成测试工具调用 AuthSvr.Register
# 例如：./bin/auth_cli --server="$AUTH_ADDR" register --username="$USERNAME" --password="$PASSWORD"

echo "[2] VerifyCredentials 获取 user_id"
# ./bin/auth_cli --server="$AUTH_ADDR" verify --username="$USERNAME" --password="$PASSWORD" > /tmp/verify.json
# USER_ID=$(jq -r .user_id /tmp/verify.json)

echo "[3] OnlineSvr.Login 签发 token"
# ./bin/online_cli --server="$ONLINE_ADDR" login --user_id="$USER_ID" --device_id="dev1" --device_type="windows" > /tmp/login.json
# TOKEN=$(jq -r .token /tmp/login.json)

echo "[4] 使用 token 再次校验（ValidateToken）"
# ./bin/online_cli --server="$ONLINE_ADDR" validate --token="$TOKEN"

echo "Auth flow test DONE."
```

```bash
# scripts/test_chat_basic.sh
# 私聊 + 群聊基础功能（通过集成测试或简易客户端驱动）

set -euo pipefail

GATE_WS_URL="${GATE_WS_URL:-ws://localhost:9090/ws}"

echo "[1] 使用两个测试账号 A/B 连接 Gate WebSocket 并执行 auth.login"
# 可使用自实现的 WebSocket CLI，如 ./bin/ws_client
# ./bin/ws_client --url="$GATE_WS_URL" --user="userA" --password="Password123!" --script=scripts/chat_scenario_a.json &
# ./bin/ws_client --url="$GATE_WS_URL" --user="userB" --password="Password123!" --script=scripts/chat_scenario_b.json &

echo "[2] A→B 发送多条消息，验证在线收消息"

echo "[3] B 断开连接，A 再发送消息，B 重连后触发 chat.pull_offline"

echo "Chat basic scenario finished, 请结合日志与客户端界面人工确认。"
```

> **基础功能测试关注指标：**
>
> - **正确性**：功能结果是否符合业务设计（好友关系状态、会话唯一性、群权限等）。
> - **稳定性**：多次重复场景是否有随机失败、崩溃、死锁。
> - **用户体验**：客户端 UI 是否及时更新（在线状态、未读数、撤回标记等）。

---

### 3. 安全测试

#### 3.1 用例总览

- **S-001 JWT 鉴权绕过**
  - 不带 token 调用受保护接口。
  - 使用伪造/篡改签名的 token。
  - 使用过期 token。
  - 预期：统一返回未认证错误；不执行业务。

- **S-002 user_id 伪造/越权**
  - 在请求体中伪造他人 user_id。
  - 预期：服务器仅信任 metadata 中 token 解出的 user_id，无法操作他人资源。

- **S-010 ZoneSvr 内网密钥校验**
  - 不带 `x-internal-secret` 或使用错误值调用 ZoneSvr。
  - 预期：拦截器拒绝请求，返回 `UNAUTHENTICATED`。

- **S-020 登出语义验证**
  - Logout 后，WebSocket 重新登录会失败（ValidateToken 失效）。
  - 业务接口在 token 未过期前仍可用（当前设计预期行为，需要与文档保持一致）。

- **S-030 输入校验 / 注入**
  - 特殊字符、超长字段、恶意 payload。
  - 预期：安全失败、日志记录，无崩溃或越权。

- **S-031 文件上传安全**
  - 超大文件、伪造扩展名（如 .jpg 中嵌可执行内容）。
  - 预期：按大小限制拒绝；不执行文件内容。

#### 3.2 安全测试脚本示例

```bash
# scripts/test_jwt_auth.sh
# 测试 JWT 鉴权与越权访问

set -euo pipefail

FRIEND_ADDR="${FRIEND_ADDR:-localhost:9096}"

VALID_TOKEN="${VALID_TOKEN:-}"   # 由正常登录流程获取
INVALID_TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.invalid.payload"

echo "[1] 不带 token 调用好友列表"
# ./bin/friend_cli --server="$FRIEND_ADDR" list_friends --no-token && echo "UNEXPECTED: should fail" || echo "OK: unauthenticated"

echo "[2] 使用非法 token 调用好友列表"
# ./bin/friend_cli --server="$FRIEND_ADDR" list_friends --token="$INVALID_TOKEN" && echo "UNEXPECTED: should fail" || echo "OK: invalid token rejected"

echo "[3] 使用合法 token 列出好友"
# ./bin/friend_cli --server="$FRIEND_ADDR" list_friends --token="$VALID_TOKEN"
```

```bash
# scripts/test_zone_internal_secret.sh
# 测试 ZoneSvr 与 GateSvr 之间的内网密钥校验

set -euo pipefail

ZONE_ADDR="${ZONE_ADDR:-localhost:9092}"

echo "[1] 使用错误 x-internal-secret 调用 ZoneSvr"
# 示例：使用自写 gRPC CLI，在 metadata 中添加错误 key
# ./bin/zone_cli --server="$ZONE_ADDR" --internal-secret="wrong_secret" ping && echo "UNEXPECTED: should be unauthenticated" || echo "OK: rejected"
```

> **安全测试关注指标：**
>
> - **鉴权严格性**：所有受保护接口必须强制检查 token；越权请求无一漏网。
> - **最小暴露面**：ZoneSvr 等仅监听内网地址；K8s 中对外不暴露敏感服务端口。
> - **错误信息**：错误响应不泄露内部实现细节（如栈信息、敏感字段）。
> - **日志审计**：安全相关失败（鉴权失败、非法输入等）有清晰日志，便于追踪。

---

### 4. 压力与性能测试

#### 4.1 压测目标与关键指标

- **目标场景**：
  - 高并发 WebSocket 连接数（GateSvr）。
  - 高 QPS 消息发送/拉取（ChatSvr + ZoneSvr）。
  - 高并发文件上传/下载（FileSvr）。
- **需要重点关注的指标：**
  - **吞吐量**：整体 QPS（请求/秒、消息/秒）。
  - **延迟**：端到端延迟 p50/p95/p99（从客户端发出到收到响应/推送）。
  - **资源占用**：各服务的 CPU / 内存 / 网络带宽 / 磁盘 IO。
  - **错误率**：超时、5xx、连接断开比例。
  - **扩展性**：增加副本后吞吐能否近似线性提升，瓶颈位置。

建议配合 Prometheus + Grafana（或 `kubectl top pods`、`htop`、`iostat` 等）观察系统指标。

#### 4.2 WebSocket 并发连接与消息路由压测

```bash
# scripts/load_ws_connections.sh
# 模拟大量 WebSocket 连接和基本消息发送

set -euo pipefail

GATE_WS_URL="${GATE_WS_URL:-ws://localhost:9090/ws}"
CONN_NUM="${CONN_NUM:-1000}"

echo "Start $CONN_NUM WebSocket connections to $GATE_WS_URL"

for i in $(seq 1 "$CONN_NUM"); do
  # 建议实现一个轻量级 ws 工具，支持简单脚本驱动
  ./bin/ws_client --url="$GATE_WS_URL" \
                  --user="user_$i" \
                  --password="Password123!" \
                  --script="scripts/ws_heartbeat_and_echo.json" &
done

wait
```

> **指标关注点：**
>
> - GateSvr 进程 CPU/内存曲线。
> - ZoneSvr QPS 与延迟（gRPC 请求时延）。
> - Redis 会话读写延迟与命中率（集群模式）。

#### 4.3 ChatSvr 消息吞吐与离线拉取压测

```bash
# scripts/load_chat_qps.sh
# 使用并发客户端对 chat.send 进行压测（可通过 Gate/Zone 或直接 gRPC）

set -euo pipefail

CHAT_TARGET="${CHAT_TARGET:-localhost:9098}"   # 或设置为 Gate/Zone 的入口
CONCURRENCY="${CONCURRENCY:-100}"
DURATION="${DURATION:-60s}"

echo "Load test chat.send with $CONCURRENCY concurrency for $DURATION"

# 示例：若实现 HTTP/gRPC 转发入口，可用 hey/wrk；否则需自写负载工具
# hey -z "$DURATION" -c "$CONCURRENCY" -m POST "http://$CHAT_TARGET/chat/send" -d @payload.json

echo "请结合服务端日志和监控查看 QPS / 延迟 / 错误率。"
```

> **指标关注点：**
>
> - ChatSvr 的写入 QPS 与平均/95/99 分位延迟。
> - RocksDB/MySQL 写入延迟和 compaction 行为（单机 vs 集群）。
> - 离线队列（Redis/RocksDB）在高压下是否出现积压或明显抖动。

#### 4.4 FileSvr 文件上传/下载压测

```bash
# scripts/load_file_upload.sh
# 文件上传压测示例

set -euo pipefail

FILE_ENDPOINT="${FILE_ENDPOINT:-http://localhost:8080/upload}"
CONCURRENCY="${CONCURRENCY:-50}"
DURATION="${DURATION:-60s}"
TEST_FILE="${TEST_FILE:-/tmp/test_10mb.bin}"

if [ ! -f "$TEST_FILE" ]; then
  echo "生成测试文件 $TEST_FILE"
  dd if=/dev/zero of="$TEST_FILE" bs=1M count=10
fi

echo "压测 FileSvr 上传接口：$FILE_ENDPOINT"

# 示例：若上传为 multipart/form-data，可使用 hey 的 -D 指定 body
# hey -z "$DURATION" -c "$CONCURRENCY" -m POST -D "$TEST_FILE" "$FILE_ENDPOINT"
```

> **指标关注点：**
>
> - FileSvr CPU / 内存 / 网络带宽占用。
> - MinIO/本地磁盘吞吐与 IO 等待时间。
> - 上传/下载失败率、超时率，是否触发重试。

#### 4.5 日志系统（AsyncLogger）对性能影响

```bash
# scripts/compare_logging_mode.sh
# 对比不同日志级别/模式下的性能

set -euo pipefail

export LOG_LEVEL=INFO
./scripts/load_chat_qps.sh
# 记录此时 QPS / 延迟 / 资源使用

export LOG_LEVEL=ERROR
./scripts/load_chat_qps.sh
# 再次记录指标，对比日志开销
```

> **指标关注点：**
>
> - 启用 AsyncLogger 时，对比同步写日志方案的性能提升（若有）。
> - 日志写线程 CPU 使用情况；是否成为瓶颈。
> - 在高压下日志系统是否导致磁盘打满或 I/O 争用。

---

### 5. 测试结果记录与回归

- **缺陷记录**：
  - 用例编号、环境（单机/Minikube）、操作步骤、实际结果、预期结果、相关日志。
- **回归策略**：
  - 缺陷修复后，至少在单机模式回归对应功能。
  - 对安全相关与高风险性能问题，在 Minikube 模式下做抽样回归。

本测试方案建议与 `docs/system.md` 保持同步更新：当系统架构或安全/性能设计发生变更时，应同步更新对应测试用例、脚本说明与关注指标。

