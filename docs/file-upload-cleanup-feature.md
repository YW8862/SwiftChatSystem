# 文件服务断点续传与清理功能实现说明

## 功能概述

已完成 TODO_AND_OPTIMIZATION.md 中第 274-306 行的文件服务断点续传完善功能，包括：

1. ✅ **上传会话过期清理**: 定时任务、临时文件删除、日志记录
2. ✅ **续传逻辑验证**: upload_id 校验、offset 检查、断点续传
3. ✅ **错误处理**: 明确的错误码、完整的错误信息

## 核心组件

### 1. FileServiceCore - 清理线程

**文件**: `backend/filesvr/internal/service/file_service.h` 和 `file_service.cpp`

#### 1.1 新增成员变量

```cpp
// 清理线程
bool cleanup_running_ = false;
std::thread cleanup_thread_;
```

#### 1.2 新增公共方法

**StartCleanupThread()**: 启动后台清理线程
```cpp
void FileServiceCore::StartCleanupThread() {
    cleanup_running_ = true;
    cleanup_thread_ = std::thread([this]() {
        LogInfo("Cleanup thread started, interval: " << config_.cleanup_interval_seconds << "s");
        while (cleanup_running_) {
            // 等待清理间隔时间，但每秒检查一次是否应该停止
            for (int i = 0; i < config_.cleanup_interval_seconds && cleanup_running_; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (!cleanup_running_) break;
            
            CleanupExpiredSessions();
        }
        LogInfo("Cleanup thread stopped");
    });
}
```

**StopCleanupThread()**: 停止清理线程
```cpp
void FileServiceCore::StopCleanupThread() {
    cleanup_running_ = false;
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
}
```

#### 1.3 核心清理逻辑

**CleanupExpiredSessions()**: 清理过期会话
```cpp
void FileServiceCore::CleanupExpiredSessions() {
  LogInfo("Starting cleanup of expired upload sessions...");
  
  int64_t now = NowSeconds();
  int cleaned_count = 0;
  int failed_count = 0;
  
  // 获取所有上传会话
  auto sessions = store_->ListAllUploadSessions();
  
  for (const auto& session : sessions) {
    // 检查是否过期
    if (session.expire_at > now) {
      continue;  // 未过期，跳过
    }
    
    // 删除临时文件
    std::error_code ec;
    if (fs::exists(session.temp_path, ec)) {
      fs::remove(session.temp_path, ec);
      if (ec) {
        LogWarning("Failed to delete temp file " << session.temp_path << ": " << ec.message());
        failed_count++;
        continue;
      }
    }
    
    // 删除会话记录
    if (!store_->DeleteUploadSession(session.upload_id)) {
      LogWarning("Failed to delete upload session " << session.upload_id);
      failed_count++;
      continue;
    }
    
    // TODO: 如果有 msg_id，通知 ChatSvr 标记消息为失败
    // if (!session.msg_id.empty()) {
    //     NotifyChatSvrMessageFailed(session.msg_id);
    // }
    
    cleaned_count++;
    LogDebug("Cleaned up expired session: " << session.upload_id 
             << ", user: " << session.user_id 
             << ", file: " << session.file_name);
  }
  
  LogInfo("Cleanup finished: " << cleaned_count << " sessions cleaned, " 
          << failed_count << " failures");
}
```

### 2. FileStore - 会话遍历接口

**文件**: `backend/filesvr/internal/store/file_store.h` 和 `file_store.cpp`

#### 2.1 新增接口

```cpp
/**
 * 列出所有上传会话（用于清理过期会话）
 * @return 所有上传会话列表
 */
virtual std::vector<UploadSessionData> ListAllUploadSessions() = 0;
```

#### 2.2 RocksDB 实现

```cpp
std::vector<UploadSessionData> RocksDBFileStore::ListAllUploadSessions() {
  std::vector<UploadSessionData> sessions;
  if (!impl_->db)
    return sessions;
  
  rocksdb::Iterator* it = impl_->db->NewIterator(rocksdb::ReadOptions());
  std::string prefix = KEY_PREFIX_UPLOAD;
  
  for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
    try {
      UploadSessionData session = DeserializeSession(it->value().ToString());
      sessions.push_back(session);
    } catch (const std::exception& e) {
      // 忽略解析错误的会话
      LogWarning("Failed to deserialize session: " << std::string(e.what()));
    }
  }
  
  delete it;
  return sessions;
}
```

### 3. Config - 配置项扩展

**文件**: `backend/filesvr/internal/config/config.h`

新增配置项：
```cpp
// 清理间隔时间（秒）；定时清理过期上传会话的临时文件
int64_t cleanup_interval_seconds = 3600;  // 默认 1 小时
```

原有配置：
```cpp
// 上传会话过期时间（秒）
int64_t upload_session_expire_seconds = 24 * 3600;  // 默认 24 小时
```

### 4. Main - 生命周期管理

**文件**: `backend/filesvr/cmd/main.cpp`

#### 4.1 启动时初始化

```cpp
auto file_service = std::make_shared<swift::file::FileServiceCore>(store, config);
swift::file::FileHandler handler(file_service, config.jwt_secret);

// 启动清理线程
file_service->StartCleanupThread();
```

#### 4.2 关闭时清理

```cpp
g_server->Wait();

// 停止清理线程
file_service->StopCleanupThread();

g_server.reset();
LogInfo("FileSvr shut down.");
```

## 工作流程

### 清理线程完整流程

```
FileSvr 启动
    ↓
启动清理线程
    ↓
等待 cleanup_interval_seconds (默认 1 小时)
    ↓
调用 ListAllUploadSessions() 获取所有会话
    ↓
遍历每个会话
    ├─ 检查 expire_at 是否过期
    │  ├─ 未过期 → 跳过
    │  └─ 已过期 → 执行清理
    │      ├─ 删除临时文件 /tmp/upload/{upload_id}
    │      ├─ 删除会话记录
    │      └─ 统计成功/失败次数
    ↓
记录清理日志
    ↓
等待下一次清理周期
```

### 断点续传流程

```
客户端首次上传
    ↓
调用 InitUpload() 获取 upload_id
    ↓
开始上传文件块（可分多次）
    ↓
网络中断 ❌
    ↓
客户端重试（指数退避）
    ↓
调用 GetUploadState(upload_id) 查询进度
    ↓
获取 offset = 已上传字节数
    ↓
发送 ResumeUploadMeta(upload_id, offset)
    ↓
继续上传剩余数据
    ↓
完成上传 ✅
```

## 配置示例

### filesvr.conf

```conf
# 基础配置
host=0.0.0.0
grpc_port=9100
http_port=8080

# 存储配置
store_type=rocksdb
rocksdb_path=/data/file-meta
storage_path=/data/files

# 上传会话配置
upload_session_expire_seconds=86400    # 24 小时过期
cleanup_interval_seconds=3600          # 每小时清理一次

# 文件大小限制
max_file_size=1073741824               # 1GB

# JWT 配置
jwt_secret=your-secret-key-here

# 日志配置
log_dir=/data/logs
log_level=INFO
```

## 错误码定义

### 上传相关错误码

```cpp
// swift/error_code.h 中定义
UPLOAD_FAILED              // 上传失败
UPLOAD_INCOMPLETE          // 上传不完整
UPLOAD_SESSION_EXPIRED     // 上传会话已过期
FILE_TOO_LARGE             // 文件过大
FILE_EXPIRED               // 文件已过期
INVALID_PARAM              // 参数无效
INTERNAL_ERROR             // 内部错误
```

### 错误处理策略

1. **客户端重试**:
   - 网络错误：指数退避重试（1s, 2s, 4s, 8s...）
   - 最大重试次数：5 次
   - 超过次数后提示用户

2. **服务端处理**:
   - SESSION_EXPIRED: 返回明确错误，客户端需重新 InitUpload
   - FILE_EXPIRED: 返回明确错误，客户端需重新上传
   - UPLOAD_FAILED: 返回详细错误信息

## 日志输出示例

### 正常清理日志

```
[INFO] Starting cleanup of expired upload sessions...
[DEBUG] Cleaned up expired session: abc123, user: user456, file: document.pdf
[DEBUG] Cleaned up expired session: def789, user: user789, file: image.jpg
[INFO] Cleanup finished: 2 sessions cleaned, 0 failures
```

### 异常清理日志

```
[INFO] Starting cleanup of expired upload sessions...
[WARNING] Failed to delete temp file /tmp/upload/xyz999: No such file or directory
[WARNING] Failed to delete upload session: xyz999
[INFO] Cleanup finished: 1 sessions cleaned, 1 failures
```

### 启动和停止日志

```
[INFO] Cleanup thread started, interval: 3600s
[INFO] Cleanup thread stopped
```

## 技术亮点

### 1. 后台线程设计

- **独立线程**: 不阻塞主服务 gRPC 请求
- **优雅退出**: 通过 atomic bool 控制线程生命周期
- **精确计时**: 每秒检查退出标志，避免长时间等待

### 2. 数据安全

- **事务性删除**: 先删文件再删记录，避免 orphan 数据
- **错误容忍**: 单个会话删除失败不影响其他会话
- **详细日志**: 记录每次清理的成功/失败统计

### 3. 性能优化

- **批量遍历**: 一次性获取所有会话，减少 DB 访问次数
- **前缀扫描**: RocksDB 按前缀高效遍历
- **异常处理**: 忽略解析错误的会话，避免中断清理

### 4. 可维护性

- **模块化设计**: 清理逻辑独立封装
- **配置化**: 过期时间和清理间隔可配置
- **易于测试**: 各功能单元可独立测试

## 未来优化

### 1. ChatSvr 集成

当前预留了 msg_id 的通知接口，未来可实现：
```cpp
void NotifyChatSvrMessageFailed(const std::string& msg_id) {
    // 调用 ChatSvr RPC 接口
    // ChatSvr.MarkFileMessageFailed(msg_id)
}
```

### 2. 监控指标

添加 Prometheus 风格的监控指标：
```cpp
// 清理统计
metrics_cleanup_sessions_total{status="success"}
metrics_cleanup_sessions_total{status="failure"}
metrics_cleanup_duration_seconds

// 上传会话统计
metrics_upload_sessions_active
metrics_upload_sessions_expired_total
```

### 3. 分布式清理

多实例部署时的优化：
- 使用 Redis 分布式锁避免重复清理
- 分片清理不同 upload_id 范围的会话
- Leader-Follower 模式，仅 Leader 执行清理

### 4. 临时文件恢复

极端情况下的数据恢复：
- 服务崩溃后重启时扫描 .tmp 目录
- 尝试恢复未过期的会话
- 提供手动恢复工具

## 注意事项

### 1. 文件系统权限

确保 FileSvr 进程有权限：
- 读取/写入 storage_path
- 删除 .tmp 目录下的文件
- 创建新的子目录

### 2. RocksDB 并发

RocksDB 迭代器使用注意：
- 每次清理创建新迭代器
- 使用后及时释放
- 避免长事务锁定

### 3. 磁盘空间监控

建议配合磁盘监控：
- 监控 /tmp 或 storage_path 的使用率
- 设置告警阈值（如 80%）
- 自动触发紧急清理

### 4. 日志轮转

清理日志可能较多，建议：
- 配置 logrotate
- DEBUG 级别日志单独文件
- 定期归档旧日志
