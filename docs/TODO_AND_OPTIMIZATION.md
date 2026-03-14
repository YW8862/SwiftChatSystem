# SwiftChatSystem 未完成功能与优化清单

**文档版本**: v1.0  
**创建时间**: 2026-03-13  
**基于文档**: docs/system.md + docs/develop.md  

---

## 📋 执行摘要

根据 system.md和 develop.md 的详细对比分析，并深入检查代码实现后，项目整体完成度约为 **70-75%**。核心微服务架构已实现，但客户端功能、监控运维、部分高级特性仍需完善。

### 关键发现（本次深入检查新增）

1. **MessageItem 组件有 3 处 TODO** - UI 完全未实现，仅骨架代码
2. **ZoneSvr 存在多个 NotImplemented 调用** - 需确认是否为有意为之或遗漏
3. **各服务缺少独立 Dockerfile** - 仅有一个通用 Dockerfile
4. **K8s 部署未实际验证** - yaml 配置存在但未测试

### 完成度总览

| 优先级 | 模块数量 | 说明 |
|--------|---------|------|
| 🔴 高 | 8 项 | 严重影响用户体验，需优先完成 |
| 🟡 中 | 10 项 | 功能可用但不够完善，可后续优化 |
| 🟢 低 | 4 项 | 锦上添花，时间充裕时实现 |
| **总计** | **22 项** | 本次深入检查新增 4 项 |

---

## 🔴 高优先级（必须完成）

### 1. Qt 客户端 - 富媒体消息支持

**现状**: 仅支持纯文本消息，图片/文件等富媒体消息 UI 未实现

**待完成**:
- [ ] **图片消息**
  - 发送图片时的预览选择对话框
  - 聊天窗口中图片缩略图展示（点击放大）
  - 图片加载进度指示器
  - 支持格式：JPG/PNG/GIF/WebP

- [ ] **文件消息**
  - 文件图标展示（按类型区分）
  - 文件名、大小显示
  - 下载按钮与进度条
  - 下载完成后打开文件

- [ ] **视频消息**（可选）
  - 视频缩略图
  - 内嵌播放器或调用外部播放器
  - 播放控制（暂停/继续）

**实现位置**:
- UI: `client/desktop/src/ui/chatwidget.cpp/h`
- 模型：`client/desktop/src/models/`
- 网络：`client/desktop/src/network/websocket_client.cpp`

**参考 proto**:
```protobuf
// common.proto
enum MessageType {
    TEXT = 0;
    IMAGE = 1;
    FILE = 2;
    VIDEO = 3;
}

message ChatMessage {
    MessageType type = 3;
    FileMetadata file_metadata = 8;  // 文件元数据
}
```

---

### 2. Qt 客户端 - @提醒与回复消息 UI

**现状**: proto 中已定义 mentions 和 reply_to_msg_id 字段，但客户端未解析展示

**待完成**:
- [ ] **@提醒功能**
  - 输入框支持 @ 符号触发用户选择
  - 被@用户在消息中以特殊颜色高亮（如蓝色）
  - 收到@消息时特殊提示（震动/铃声/红点）
  - 点击@链接跳转到对应消息

- [x] **回复消息功能** ✅
  - 右键菜单"回复"某条消息
  - 输入框上方显示"回复 XXX: 原消息内容"
  - 消息气泡中展示引用块（灰色背景）
  - 点击引用块跳转到原消息

**实现位置**:
- 输入框：`ChatWidget::onSendMessage()`
- 消息展示：`MessageItem` 类扩展
- 协议解析：`ProtocolHandler::handleChatMessage()`

**参考 proto**:
```protobuf
// zone.proto (客户端协议)
message ChatSendPayload {
    string conversation_id = 2;
    string content = 3;
    repeated string mentions = 5;        // @的用户 ID 列表
    optional string reply_to_msg_id = 6; // 回复的消息 ID
}

message ChatMessagePushPayload {
    // ... 同上
}
```

---

### 3. Qt 客户端 - 已读回执 UI

**现状**: 服务端已实现 MarkRead 和推送逻辑，客户端已展示

**完成情况**:
- [x] **已读状态展示** ✅
  - 私聊：消息下方显示"XXX 已读"
  - 群聊：消息下方显示"X 人已读，Y 人未读"
  - 点击可查看已读/未读成员列表

- [x] **已读回执推送处理** ✅
  - 接收 `cmd="chat.read_receipt"` 推送
  - 更新对应消息的已读状态
  - UI 刷新（不突兀）

**实现位置**:
- 消息模型扩展：已有 `replyToMsgId` 字段
- `ChatWidget` 增加已读详情对话框：`ReadReceiptDetailDialog`
- `ProtocolHandler` 已有 `handleReadReceipt()` 推送处理
- `MainWindow::onPushReadReceipt()` 处理已读推送

**参考 proto**:
```protobuf
// gate.proto
message ReadReceiptNotify {
    string chat_id = 1;          // 会话 ID
    int32 chat_type = 2;         // 私聊/群聊
    string user_id = 3;          // 谁已读
    string last_read_msg_id = 4; // 最后读到的消息 ID
}
```

---

### 4. Qt 客户端 - 离线消息拉取

**现状**: 服务端已实现 PullOffline RPC，客户端未调用

**待完成**:
- [ ] **自动拉取逻辑**
  - 登录成功后立即调用 `chat.pull_offline`
  - 网络恢复时自动拉取
  - 手动下拉刷新触发拉取

- [ ] **分页加载**
  - 首次拉取 limit=50 条
  - has_more=true 时显示"加载更多"按钮
  - 使用 next_cursor 作为分页游标

- [ ] **消息去重与合并**
  - 避免重复插入同一 msg_id
  - 按 msg_id 排序保证时间线正确

**实现位置**:
- `LoginWindow::onLoginSuccess()` 增加拉取逻辑
- `ChatWidget` 增加下拉刷新手势识别
- `AppService` 增加 pullOffline() 接口

**参考 proto**:
```protobuf
// zone.proto
message ChatPullOfflinePayload {
    int32 limit = 1;
    string cursor = 2;
}

message ChatPullOfflineResponsePayload {
    repeated ChatMessagePushPayload messages = 1;
    string next_cursor = 2;
    bool has_more = 3;
}
```

---

### 5. Qt 客户端 - 群管理功能 UI

**现状**: 服务端 GroupSystem 已实现创建群、邀请、踢人等，客户端已有完整 UI

**完成情况**:
- [x] **创建群聊** ✅
  - 联系人列表右键"创建群聊"
  - 选择至少 3 人（少于 3 人禁用确定按钮）
  - 输入群名称、群头像
  - 调用 `group.create`

- [x] **群成员管理** ✅
  - 群聊窗口右侧成员列表
  - 群主/管理员可见"邀请成员"按钮
  - 群主可见"移出群聊"按钮（右键成员）
  - 群主可见"转让群主"按钮

- [x] **群设置** ✅
  - 群公告编辑与展示（单独 Tab 页）
  - 修改群名称、头像（仅群主/管理员）
  - 查看群信息（创建时间、成员数）

- [x] **入群通知** ✅
  - 新成员加入时系统消息
  - 被踢出群聊时提示

**实现位置**:
- `ContactWidget`: 群列表、群成员列表、群信息卡片
- `MainWindow`: `createGroup()`, `inviteGroupMembers()`, `removeGroupMember()`, `leaveGroup()`, `dismissGroup()`
- 右键菜单：群聊窗口的"..."按钮提供邀请、踢人、退出、删除等功能

**参考 proto**:
```protobuf
// group.proto
message CreateGroupRequest {
    string name = 1;
    repeated string member_ids = 2;  // 至少 3 人
    string avatar_url = 3;
}

message InviteMemberRequest {
    string group_id = 1;
    repeated string user_ids = 2;
}

message KickMemberRequest {
    string group_id = 1;
    string member_id = 2;
}
```

---

### 6. Qt 客户端 - 好友申请与黑名单 UI

**现状**: FriendSvr 已实现完整 CRUD，客户端已有完整交互界面

**完成情况**:
- [x] **好友申请** ✅
  - 搜索用户（按用户名/ID）
  - 发送加好友请求（附言）
  - 接收申请弹窗（同意/拒绝）
  - 申请记录列表

- [x] **好友分组** ✅
  - 自定义分组（家人、同事、朋友等）
  - 拖拽移动好友到不同分组
  - 分组展开/折叠

- [x] **黑名单管理** ✅
  - 添加/移除黑名单
  - 黑名单用户发消息时提示"已将对方加入黑名单"
  - 黑名单列表展示

**实现位置**:
- `MainWindow::showAddFriendDialog()`: 搜索并添加好友
- `ContactWidget`: 好友申请列表展示和處理
- `BlacklistDialog`: 黑名单管理对话框
- `MainWindow::blockUser()` / `unblockUser()`: 拉黑/取消拉黑逻辑

---

## 🟡 中优先级（应该完成）

### 7. 文件服务 - 断点续传完善

**现状**: InitUpload/UploadFile 已实现，超时清理和续传逻辑已完成 ✅

**完成情况**:
- [x] **上传会话过期清理** ✅
  - 定时任务扫描 expire_at < now 的会话
  - 删除临时文件（`/tmp/upload/{upload_id}`）
  - 预留 msg_id 通知 ChatSvr 接口

- [x] **续传逻辑验证** ✅
  - ResumeUpload 首条消息校验 upload_id 有效性
  - 检查已上传 chunk 的 offset 和 md5
  - 从正确位置继续接收

- [x] **错误处理** ✅
  - 网络中断后客户端自动重试（指数退避）
  - 服务端返回明确的错误码（UPLOAD_SESSION_EXPIRED, FILE_EXPIRED 等）

**实现位置**:
- `backend/filesvr/internal/service/file_service.cpp`: 清理线程和过期会话清理逻辑
- `backend/filesvr/internal/store/file_store.h/cpp`: ListAllUploadSessions 接口
- `backend/filesvr/cmd/main.cpp`: 启动和停止清理线程

**配置项**:
```conf
# filesvr.conf
upload_session_expire_seconds=86400  # 24 小时
cleanup_interval_seconds=3600        # 每小时清理
```

**核心功能**:
- `FileServiceCore::StartCleanupThread()`: 启动后台清理线程
- `FileServiceCore::CleanupExpiredSessions()`: 清理过期会话
- `FileStore::ListAllUploadSessions()`: 遍历所有上传会话
- 自动删除临时文件和会话记录
- 详细的日志记录和错误统计

---

### 8. 异步写中间层（性能优化）

**现状**: `swift::async::IWriteExecutor` 接口已定义但未使用

**待完成**:
- [ ] **实现 AsyncDbWriteExecutor**
  - 内部维护写队列（无锁环形缓冲）
  - 写线程池（4 线程）批量提交
  - 按 key 哈希分片，避免冲突

- [ ] **接入 FileSvr**
  - AppendChunk 改为 `executor->Submit(write_task)`
  - 非核心路径：fire-and-forget

- [ ] **接入 ChatSvr**
  - MessageStore::Save 改为 `executor->SubmitAndWait(save_task)`
  - 强一致要求：阻塞等待落盘

**参考设计**:
```cpp
// backend/common/include/swift/async_write.h
namespace swift::async {

class IWriteExecutor {
public:
    virtual void Submit(WriteTask task) = 0;
    virtual WriteResult SubmitAndWait(WriteTask task) = 0;
};

// 未来实现
class AsyncDbWriteExecutor : public IWriteExecutor {
private:
    std::vector<std::unique_ptr<WriteThread>> workers_;
    LockFreeQueue<WriteTask> queue_;
};

}  // namespace swift::async
```

**注意**: 当前同步写在低并发下足够，引入异步写会增加复杂性，建议压测确认瓶颈后再实施。

---

### 9. 监控指标暴露

**现状**: 无任何 metrics 端点

**待完成**:
- [ ] **基础指标收集**
  - QPS（每服务）
  - 请求延迟（P50/P90/P99）
  - gRPC 连接数
  - WebSocket 连接数
  - RocksDB 读写延迟
  - 内存使用量

- [ ] **Prometheus Exporter**
  - 各服务增加 `/metrics` HTTP 端点（端口 +10000，如 9094→19094）
  - 使用 prometheus-cpp 库

- [ ] **Grafana 仪表盘**
  - 导入 Node Exporter 模板
  - 自定义 SwiftChat 仪表盘（JSON 配置）

**实现位置**:
- 各服务 main.cpp 增加 HTTP Server（Boost.Beast 轻量级）
- `backend/common` 封装 MetricsCollector 单例

**配置**:
```conf
# 各服务通用
enable_metrics=true
metrics_port_offset=10000
```

---

### 10. Docker 镜像与 compose 编排

**现状**: 部分 Dockerfile 可能缺失，docker-compose.yml 需验证

**待完成**:
- [ ] **补充 Dockerfile**
  - 检查每个服务是否有 Dockerfile（authsvr/onlinesvr/friendsvr/chatsvr/filesvr/zonesvr/gatesvr）
  - 统一使用多阶段构建（builder + runtime）
  - 基于 ubuntu:22.04 或 alpine

- [ ] **docker-compose.yml**
  - 定义所有服务 + Redis + MySQL
  - 配置依赖关系（depends_on）
  - 挂载配置文件（./config:/app/config）
  - 健康检查（healthcheck）

**示例**:
```yaml
version: '3.8'
services:
  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"
  
  authsvr:
    build:
      context: .
      dockerfile: backend/authsvr/Dockerfile
    environment:
      - STORE_TYPE=rocksdb
      - ROCKSDB_PATH=/data/auth
    volumes:
      - ./data/auth:/data
    depends_on:
      - redis
```

---

### 11. Kubernetes 部署验证

**现状**: deploy/k8s/下有 yaml 配置，但未实际测试

**待完成**:
- [ ] **Headless Service 配置**
  - 检查所有服务是否有对应的 Headless Service（clusterIP: None）
  - 验证 DNS 解析是否正常

- [ ] **gRPC 负载均衡**
  - 客户端 channel 参数：`grpc::ChannelArguments().SetLoadBalancingPolicyName("round_robin")`
  - 验证多副本是否轮询

- [ ] **ConfigMap 与 Secret**
  - 将 .conf 文件放入 ConfigMap
  - jwt_secret 等敏感信息用 Secret

- [ ] **HPA 自动扩缩**（可选）
  - CPU 利用率 > 70% 时扩容
  - 最小 2 副本，最大 10 副本

**验证脚本**:
```bash
#!/bin/bash
# deploy/k8s/verify.sh
kubectl apply -k deploy/k8s/
sleep 30
kubectl get pods -n swift
kubectl get svc -n swift
# 测试 gRPC 调用
grpcurl -plaintext authsvr.swift.svc.cluster.local:9094 list
```

---

### 12. 集成测试脚本

**现状**: 无端到端测试

**待完成**:
- [ ] **API 测试**
  - 注册 → 登录 → 加好友 → 发消息 → 撤回 → 登出
  - 使用 grpcurl 或自研 CLI 工具

- [ ] **UI 自动化测试**（可选）
  - Qt Test 框架
  - 模拟用户操作：点击、输入、拖拽

- [ ] **压力测试**
  - 模拟 1000 个并发用户同时发消息
  - 测量 P99 延迟和吞吐量

**测试框架**:
```bash
# scripts/run-integration-tests.sh
# 使用 Python + grpcio-tools
pip install grpcio-tools
python tests/integration/test_chat_flow.py
```

---

## 🟢 低优先级（可选优化）

### 13. 消息本地搜索

**现状**: 客户端无搜索界面

**待完成**:
- [ ] **会话内搜索**
  - Ctrl+F 唤起搜索框
  - 输入关键词高亮匹配消息
  - 上一条/下一条导航

- [ ] **全局搜索**（可选）
  - 搜索所有会话包含关键词的消息
  - 按时间/会话排序结果

**实现**: 前端遍历已加载的历史消息，无需服务端支持

---

### 14. 语音消息（可选）

**待完成**:
- [ ] 录音功能（按住说话）
- [ ] 语音播放（点击播放）
- [ ] 语音时长显示
- [ ] 自动播放下一句（可选）

**注意**: 需额外依赖音频编解码库（Opus/MP3）

---

### 15. 主题与个性化

**待完成**:
- [ ] 深色模式切换
- [ ] 聊天背景自定义
- [ ] 字体大小调节
- [ ] 消息气泡颜色主题

---

### 16. 运维文档完善

**待完成**:
- [ ] **故障排查手册**
  - 常见错误码及解决方案
  - 日志分析方法
  - 性能问题定位流程

- [ ] **备份恢复方案**
  - RocksDB/MySQL 定期备份脚本
  - 灾难恢复演练步骤

- [ ] **性能调优指南**
  - RocksDB 参数优化
  - gRPC 连接池大小建议
  - Redis 内存优化

---

## 🔍 其他潜在问题与优化点

### 17. ZoneSvr - 未实现的命令处理

**现状**: `zone_service.cpp` 中存在多个 `return NotImplemented(cmd, request_id)` 调用

**待完成**:
- [x] **auth.* 命令完整性检查** ✅
  - 当前已实现：`auth.register`, `auth.login`, `auth.logout`, `auth.validate_token`
  - 状态：已全部实现，无遗漏

- [x] **chat.* 命令完整性检查** ✅
  - 当前已实现：`chat.send_message`, `chat.mark_read`, `chat.pull_offline`, `chat.recall_message`, `chat.get_history`, `chat.sync_conversations`, `chat.delete_conversation`
  - 状态：核心命令已全部实现

- [x] **friend.* 命令路由** ✅
  - 已实现：`friend.add`, `friend.handle_request`, `friend.remove`, `friend.block`, `friend.unblock`, `friend.get_friends`, `friend.get_requests`, `friend.search`
  - 新增实现：`friend.get_block_list`, `friend.create_group`, `friend.get_groups`, `friend.move_to_group`, `friend.delete_group`, `friend.set_remark`
  - 状态：全部实现

- [x] **group.* 命令路由** ✅
  - 已实现：`group.create`, `group.dismiss`, `group.invite_members`, `group.remove_member`, `group.leave`, `group.get_info`, `group.get_members`, `group.get_user_groups`
  - 状态：全部实现

- [x] **file.* 命令路由** ✅
  - 已实现：`file.get_upload_token`, `file.get_file_url`, `file.delete`
  - 新增实现：`file.init_upload`, `file.get_file_info`
  - 注：`file.upload_file` 为流式上传，通过 HTTP/gRPC 流直接处理，不在 HandleClientRequest 中处理
  - 状态：全部实现

**完成情况**:
- ✅ 所有核心命令已在 `zone_service.cpp` 中实现
- ✅ 新增 Proto 定义：`FileInitUploadPayload`, `FileGetFileInfoPayload`, `FriendGetBlockListPayload`, `FriendCreateGroupPayload`, `FriendGetGroupsPayload`, `FriendMoveToGroupPayload`, `FriendDeleteGroupPayload`, `FriendSetRemarkPayload` 等
- ✅ 新增 RPC 客户端方法：`FileRpcClient::GetFileInfo`, `FriendRpcClient::CreateFriendGroup`, `FriendRpcClient::GetFriendGroups`, `FriendRpcClient::MoveFriendToGroup`, `FriendRpcClient::DeleteFriendGroup`, `FriendRpcClient::SetRemark`
- ✅ 新增 System 层接口：`FileSystem::InitUpload`, `FileSystem::GetFileInfo`, `FriendSystem::GetBlockList`, `FriendSystem::CreateFriendGroup`, `FriendSystem::GetFriendGroups`, `FriendSystem::MoveFriendToGroup`, `FriendSystem::DeleteFriendGroup`, `FriendSystem::SetRemark`
- ✅ 编译测试通过

**实现位置**:
- `backend/zonesvr/internal/service/zone_service.cpp` - HandleFile, HandleFriend 方法
- `backend/zonesvr/internal/system/file_system.h/.cpp` - InitUpload, GetFileInfo
- `backend/zonesvr/internal/system/friend_system.h/.cpp` - 新增好友分组和黑名单管理方法
- `backend/zonesvr/internal/rpc/file_rpc_client.h/.cpp` - GetFileInfo
- `backend/zonesvr/internal/rpc/friend_rpc_client.h/.cpp` - 好友分组相关方法
- `backend/zonesvr/proto/zone.proto` - 新增消息定义

---

### 18. 客户端 - MessageItem 组件未完成

**现状**: `messageitem.cpp` 中有 3 处 TODO 标记，UI 未实际渲染

**待完成**:
- [ ] **消息气泡 UI 布局**
  - 自己发送的消息：靠右对齐，绿色背景
  - 他人发送的消息：靠左对齐，白色背景
  - 包含：头像、昵称、时间戳、消息内容

- [ ] **消息类型渲染**
  - 文本消息：QLabel 显示 content
  - 图片消息：QLabel + QPixmap 显示缩略图
  - 文件消息：图标 + 文件名 + 大小 + 下载按钮
  - 撤回消息：灰色文字"XXX 撤回了一条消息"

- [ ] **交互功能**
  - 右键菜单：复制、撤回、删除、回复
  - 点击头像：查看用户资料
  - 点击图片：放大预览
  - 链接检测：URL 自动可点击

**实现位置**:
- `client/desktop/src/ui/messageitem.cpp/h`

**示例代码框架**:
```cpp
void MessageItem::setMessage(const QString& msgId, const QString& content,
                              const QString& senderName, bool isSelf, qint64 timestamp) {
    m_msgId = msgId;
    m_isSelf = isSelf;
    
    // 创建布局
    auto* layout = new QHBoxLayout(this);
    layout->setSpacing(10);
    
    // 头像
    auto* avatar = new QLabel(this);
    avatar->setFixedSize(40, 40);
    avatar->setPixmap(QPixmap(":/avatars/default.png").scaled(40, 40, Qt::KeepAspectRatioByExpanding));
    
    // 消息气泡
    auto* bubble = new QFrame(this);
    bubble->setLayout(new QVBoxLayout());
    bubble->setStyleSheet(QString("background-color: %1; border-radius: 8px;")
                         .arg(isSelf ? "#95EC69" : "#FFFFFF"));
    
    // 昵称
    auto* nameLabel = new QLabel(senderName, bubble);
    nameLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
    
    // 内容
    auto* contentLabel = new QLabel(content, bubble);
    contentLabel->setWordWrap(true);
    contentLabel->setStyleSheet("font-size: 14px; padding: 8px;");
    
    // 时间戳
    auto* timeLabel = new QLabel(QDateTime::fromMSecsSinceEpoch(timestamp).toString("HH:mm"), bubble);
    timeLabel->setStyleSheet("font-size: 11px; color: #999;");
    
    // 组装
    if (isSelf) {
        layout->addStretch();
        layout->addWidget(bubble);
        layout->addWidget(avatar);
    } else {
        layout->addWidget(avatar);
        layout->addWidget(bubble);
        layout->addStretch();
    }
}
```

---

### 19. Dockerfile 缺失

**现状**: 仅 `deploy/docker/Dockerfile` 一个通用 Dockerfile，各服务无独立 Dockerfile

**影响**:
- 无法单独构建某个服务的镜像
- 无法使用多阶段构建优化镜像大小
- 生产环境部署不便

**待完成**:
- [ ] **为每个服务创建 Dockerfile**
  - `backend/authsvr/Dockerfile`
  - `backend/onlinesvr/Dockerfile`
  - `backend/friendsvr/Dockerfile`
  - `backend/chatsvr/Dockerfile`
  - `backend/filesvr/Dockerfile`
  - `backend/zonesvr/Dockerfile`
  - `backend/gatesvr/Dockerfile`

- [ ] **统一使用多阶段构建**
```dockerfile
# 编译阶段
FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y \
    build-essential cmake libssl-dev \
    protobuf-compiler-grpc libgrpc++-dev \
    libboost-all-dev librocksdb-dev
COPY . /src
WORKDIR /src/build
cmake .. && make -j4

# 运行阶段
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    libssl-dev libprotobuf-dev \
    && rm -rf /var/lib/apt/lists/*
COPY --from=builder /src/build/backend/authsvr/authsvr /app/authsvr
COPY config/authsvr.conf.example /app/config/authsvr.conf
WORKDIR /app
EXPOSE 9094
CMD ["./authsvr"]
```

---

### 20. 配置管理优化

**现状**: ✅ 已完成 - 每个服务都有独立的 Dockerfile，支持灵活构建

**已实现功能**:
- ✅ **为每个服务创建独立 Dockerfile**
  - `backend/authsvr/Dockerfile`
  - `backend/onlinesvr/Dockerfile`
  - `backend/friendsvr/Dockerfile`
  - `backend/chatsvr/Dockerfile`
  - `backend/filesvr/Dockerfile`
  - `backend/zonesvr/Dockerfile`
  - `backend/gatesvr/Dockerfile`

- ✅ **统一使用多阶段构建**
  - 编译阶段：安装全部开发依赖，编译源代码
  - 运行阶段：仅包含运行时依赖，镜像体积更小
  - 所有服务均采用相同的构建流程，便于维护

- ✅ **Makefile 支持灵活的构建命令**
```bash
# 构建所有后端服务镜像
make docker/all
make build/all          # 同 docker/all

# 构建单个服务镜像
make authsvr            # 构建 swift/authsvr:latest
make onlinesvr          # 构建 swift/onlinesvr:latest
make friendsvr          # 构建 swift/friendsvr:latest
make chatsvr            # 构建 swift/chatsvr:latest
make filesvr            # 构建 swift/filesvr:latest
make zonesvr            # 构建 swift/zonesvr:latest
make gatesvr            # 构建 swift/gatesvr:latest

# 客户端构建
make client             # 构建 Linux 客户端
make client-windows     # 构建 Windows 客户端（交叉编译）
make docker/client      # 构建客户端 Docker 镜像

# 清理
make clean              # 清理构建产物
docker-clean            # 删除所有 SwiftChat 相关镜像
```

- ✅ **保留原有统一 Dockerfile**
  - `deploy/docker/Dockerfile` 仍然可用
  - 通过 `--build-arg BUILD_TARGET=xxx` 参数选择构建的服务
  - 适合快速测试或一次性构建多个服务

**Dockerfile 示例结构** (以 authsvr 为例):
```dockerfile
# AuthSvr 服务 Docker 镜像构建
# 使用多阶段构建优化镜像大小
# 从仓库根目录构建：docker build -f backend/authsvr/Dockerfile -t swift/authsvr:latest .

ARG BASE_IMAGE=docker.1ms.run/library/ubuntu:22.04

# ========== 编译阶段 ==========
FROM ${BASE_IMAGE} AS builder

# 安装构建工具和依赖
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git ... \
    libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
    librocksdb-dev ... \
    && rm -rf /var/lib/apt/lists/*

# jwt-cpp（头文件库）
RUN git clone --depth 1 https://gitee.com/mirrors/jwt-cpp.git /opt/jwt-cpp || \
    git clone --depth 1 https://ghproxy.com/https://github.com/Thalhammer/jwt-cpp.git /opt/jwt-cpp

WORKDIR /app
COPY . .

# 编译 authsvr
RUN mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-I/opt/jwt-cpp/include" \
        -DBUILD_AUTHSVR_TESTS=OFF ... \
    && make authsvr -j$(nproc) \
    && cp /app/build/backend/authsvr/authsvr /app/outbin

# ========== 运行阶段 ==========
FROM ${BASE_IMAGE}

# 运行时依赖（精简）
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates libstdc++6 curl iproute2 procps netcat-openbsd \
    libgrpc++-dev libprotobuf-dev librocksdb-dev ... \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/outbin ./authsvr
COPY config/authsvr.conf.example ./config/authsvr.conf

RUN mkdir -p /data/logs /data/data && chmod +x /app/authsvr
EXPOSE 9094
CMD ["./authsvr"]
```

**优势对比**:
| 特性 | 旧方案 (单一 Dockerfile) | 新方案 (独立 Dockerfile) |
|------|------------------------|------------------------|
| 单独构建 | ❌ 不支持 | ✅ 支持 (`make authsvr`) |
| 并行构建 | ❌ 不支持 | ✅ 支持 (`make docker/all`) |
| 镜像优化 | ⚠️ 一般 | ✅ 多阶段构建，体积更小 |
| 可维护性 | ⚠️ 集中修改，易冲突 | ✅ 各自独立，互不影响 |
| 灵活性 | ⚠️ 低 | ✅ 高 (可为不同服务定制) |
| 向后兼容 | - | ✅ 保留原统一 Dockerfile |

**使用场景**:
1. **开发环境**: `make authsvr` 快速构建单个服务镜像
2. **CI/CD**: `make docker/all` 并行构建所有服务镜像
3. **生产部署**: 每个服务独立版本控制，按需更新
4. **调试排障**: 镜像内置调试工具 (curl/ss/ps/nc)

---

### 20. 配置管理优化

**现状**: 基础鉴权已实现，但仍有改进空间

**待完成**:
- [ ] **防重放攻击**
  - JWT 中加入 nonce 或 timestamp
  - 服务端缓存已使用的 token（短时间内）

- [ ] **敏感信息加密**
  - 聊天记录端到端加密（可选）
  - 数据库中手机号/邮箱加密存储

- [ ] **限流与防刷**
  - 单用户每秒最多发 5 条消息
  - 注册接口 IP 维度限流（10 次/分钟）
  - 使用令牌桶算法

**实现位置**:
- GateSvr 增加 RateLimiter
- ZoneSvr 增加 RequestThrottle

---

### 18. 容错与灾备

**现状**: 单点故障风险

**待完成**:
- [ ] **服务宕机自动重启**
  - K8s livenessProbe/readinessProbe
  - 进程崩溃自动拉起（systemd 或 supervisord）

- [ ] **数据持久化**
  - RocksDB WAL 模式开启
  - MySQL 主从复制
  - Redis AOF 持久化

- [ ] **降级策略**
  - FileSvr 不可用时，允许只发文本消息
  - 离线消息队列满时，丢弃最早的消息

---

### 19. 性能优化

**现状**: 未进行系统性压测

**潜在瓶颈**:
- [ ] **RocksDB 写入放大**
  - 启用压缩（LZ4/Snappy）
  - 调整 memtable_size 和 write_buffer_size

- [ ] **gRPC 序列化开销**
  - 大消息分片（>1MB 拆分）
  - 使用 arena 分配减少内存拷贝

- [ ] **WebSocket 连接数限制**
  - 单 GateSvr 最多 1 万连接
  - 超过后水平扩展 Gate 实例

**优化建议**:
```cpp
// RocksDB 配置优化
rocksdb::Options options;
options.compression = rocksdb::kLZ4Compression;
options.write_buffer_size = 64 * 1024 * 1024;  // 64MB
options.max_write_buffer_number = 4;
```

---

### 20. 代码质量

**现状**: 单元测试覆盖率不足

**待完成**:
- [ ] **提高测试覆盖率**
  - 目标：核心模块 > 80%
  - 使用 gcov/lcov生成覆盖率报告

- [ ] **静态代码分析**
  - clang-tidy 集成到 CI
  - cppcheck 扫描潜在 bug

- [ ] **代码格式化**
  - clang-format 统一风格
  - pre-commit hook 自动格式化

---

### 21. 日志与追踪

**现状**: AsyncLogger 已实现，但缺乏分布式追踪

**待完成**:
- [ ] **链路追踪**
  - 引入 OpenTelemetry 或 Zipkin
  - 每个请求生成 trace_id，跨服务传递
  - 可视化调用链

- [ ] **结构化日志**
  - JSON 格式输出（便于 ELK 收集）
  - 关键字段：user_id, request_id, duration

**示例**:
```json
{
  "timestamp": "2026-03-13T10:30:00Z",
  "level": "INFO",
  "service": "ChatSvr",
  "trace_id": "abc123",
  "span_id": "def456",
  "event": "message_sent",
  "user_id": "u_789",
  "msg_id": "m_101112",
  "duration_ms": 15
}
```

---

### 22. 配置管理优化

**现状**: 使用 .conf + 环境变量

**待完成**:
- [ ] **配置热重载**
  - SIGHUP 信号触发重新加载配置
  - 无需重启服务

- [ ] **配置校验**
  - 启动时检查必填项
  - 非法值给出明确错误

- [ ] **默认配置**
  - 所有配置项有合理默认值
  - 开发环境可直接启动无需配置

---

### 23. 数据库迁移工具

**现状**: RocksDB → MySQL 迁移需手动

**待完成**:
- [ ] **数据导出工具**
  - RocksDB 全量扫描导出为 JSON/CSV

- [ ] **数据导入工具**
  - CSV 导入 MySQL
  - 冲突处理策略（跳过/覆盖）

- [ ] **双写迁移**（平滑升级）
  - 同时写 RocksDB 和 MySQL
  - 校验一致性后切换读路径

---

### 24. 客户端安装包

**现状**: Windows 客户端编译产出 .exe，但无安装程序

**待完成**:
- [ ] **NSIS 安装包**
  - 一键安装向导
  - 创建桌面快捷方式
  - 开机自启动选项

- [ ] **自动更新**
  - 启动时检查新版本
  - 后台下载增量包
  - 重启后应用更新

**脚本**:
```nsis
; scripts/client-installer.nss
!include "MUI2.nsh"
Name "SwiftChat v1.0"
OutFile "SwiftChat-Setup.exe"
Section "Install"
  SetOutPath "$INSTDIR"
  File client\desktop\build\Release\SwiftChat.exe
  CreateShortcut "$DESKTOP\SwiftChat.lnk" "$INSTDIR\SwiftChat.exe"
SectionEnd
```

---

## 📊 工作量估算

| 优先级 | 任务数 | 预计工时（人天） |
|--------|-------|----------------|
| 🔴 高 | 8 | 35-40 天（新增 MessageItem 和 Dockerfile 任务） |
| 🟡 中 | 10 | 25-30 天 |
| 🟢 低 | 4 | 10-15 天 |
| **总计** | **22** | **70-85 天** |

按单人开发计算，约需 **3.5-4 个月**完成所有功能。若团队 3 人并行开发，可缩短至 **1.5-2 个月**。

---

## 🎯 推荐里程碑

### Phase 1（2 周）- 核心体验完善
- ✅ 离线消息拉取
- ✅ @提醒与回复消息
- ✅ 已读回执展示

### Phase 2（2 周）- 群聊与社交
- ✅ 群管理功能
- ✅ 好友申请与分组
- ✅ 黑名单管理

### Phase 3（1 周）- 富媒体支持
- ✅ 图片/文件消息
- ✅ 上传进度与断点续传

### Phase 4（1 周）- 部署与测试
- ✅ Docker compose 编排
- ✅ K8s 部署验证
- ✅ 集成测试脚本

### Phase 5（可选）- 监控与优化
- ✅ Prometheus 监控
- ✅ 性能压测与优化
- ✅ 异步写中间层

---

## 📝 附录

### A. Proto 字段完整性检查

| Proto 文件 | 已实现 | 待实现 |
|-----------|-------|--------|
| common.proto | ✅ 100% | - |
| gate.proto | ✅ 100% | - |
| zone.proto | ✅ 100% | - |
| auth.proto | ✅ 100% | - |
| online.proto | ✅ 100% | - |
| friend.proto | ✅ 100% | - |
| chat.proto | ✅ 100% | - |
| group.proto | ✅ 100% | - |
| file.proto | ✅ 100% | - |

**结论**: 所有 proto 定义已完成，剩余工作集中在客户端 UI 和服务端逻辑完善。

---

### B. 服务端接口实现率

| 服务 | Handler 方法数 | 已实现 | 实现率 |
|------|--------------|--------|--------|
| AuthSvr | 4 | 4 | 100% |
| OnlineSvr | 3 | 3 | 100% |
| FriendSvr | 6 | 6 | 100% |
| ChatSvr | 8 | 8 | 100% |
| GroupSvr | 6 | 6 | 100% |
| FileSvr | 4 | 4 | 100% |
| ZoneSvr | 5 (Systems) | 5 | 100% |
| GateSvr | 3 | 3 | 100% |

**结论**: 服务端核心接口已全部实现，剩余工作为边界情况处理和性能优化。

---

### C. 客户端页面完成度

| 页面/组件 | 状态 | 完成度 |
|----------|------|--------|
| LoginWindow | ✅ 完成 | 95% |
| MainWindow | ✅ 完成 | 90% |
| ChatWidget | ⚠️ 部分 | 70% |
| ContactWidget | ⚠️ 部分 | 65% |
| MessageItem | ❌ 未完成 | 10% (仅骨架，UI 未实现) |
| GroupManagementDialog | ❌ 缺失 | 0% |
| FriendRequestDialog | ❌ 缺失 | 0% |
| BlacklistDialog | ❌ 缺失 | 0% |

**结论**: 客户端 UI 是主要短板，需重点投入。MessageItem 作为核心组件仍有 3 处 TODO 待实现。

---

### D. ZoneSvr 命令处理完整性

| Handler | 已实现命令 | 未实现/待确认 |
|---------|-----------|--------------|
| HandleAuth | register, login, logout, validate_token | 需确认是否有遗漏 |
| HandleChat | send_message, mark_read, pull_offline | get_history, recalled 等 |
| HandleFriend | 待检查 | 需完整梳理所有 friend.* 命令 |
| HandleGroup | 待检查 | 需完整梳理所有 group.* 命令 |
| HandleFile | 待检查 | 需完整梳理所有 file.* 命令 |

**结论**: ZoneService 中存在多个`NotImplemented`调用，需逐一确认是否为有意为之或遗漏实现。

---

### E. Docker 与部署

| 项目 | 状态 | 说明 |
|------|------|------|
| 通用 Dockerfile | ✅ 存在 | `deploy/docker/Dockerfile` |
| 服务独立 Dockerfile | ❌ 缺失 | 7 个服务均需独立 Dockerfile |
| docker-compose.yml | ⚠️ 待验证 | 需测试一键启动所有服务 |
| K8s yaml | ✅ 存在 | `deploy/k8s/` 下配置需实际部署验证 |
| Minikube 测试 | ❌ 未执行 | 需在 Minikube 中实际运行验证 |

**结论**: Docker 镜像构建和容器化部署需完善，特别是多阶段构建和多副本测试。

---

**文档结束**

---

**下一步行动**:
1. 召集团队评审本清单，确定优先级
2. 创建 Jira/Trello 看板，拆分为具体任务
3. 分配开发人员，开始 Phase 1 开发
4. 每周跟踪进度，动态调整优先级

**联系方式**: 如有遗漏或建议，请提交 Issue 或 PR。
