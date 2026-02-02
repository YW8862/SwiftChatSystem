/**
 * @file system_manager.h
 * @brief System 管理器
 * 
 * SystemManager 负责：
 * - 创建和管理所有 System 实例
 * - 统一初始化和关闭所有 System
 * - 根据 cmd 分发请求到对应 System
 * 
 * 架构说明（API Gateway 模式）：
 * 
 *   GateSvr
 *      │
 *      ▼
 *   ZoneSvr (统一入口)
 *      │
 *      ├── AuthSystem ──(RPC)──→ AuthSvr (实际逻辑)
 *      ├── ChatSystem ──(RPC)──→ ChatSvr (实际逻辑)
 *      ├── FriendSystem ──(RPC)──→ FriendSvr (实际逻辑)
 *      ├── GroupSystem ──(RPC)──→ ChatSvr/GroupService
 *      └── FileSystem ──(RPC)──→ FileSvr (实际逻辑)
 * 
 * System 只负责 RPC 转发，实际业务逻辑在后端服务中实现。
 * 如果后端服务需要调用其他服务，由后端服务自己持有对应的 Client。
 */

#pragma once

#include <memory>
#include <string>

#include "base_system.h"

namespace swift {
namespace zone {

// 前向声明
class AuthSystem;
class ChatSystem;
class FriendSystem;
class GroupSystem;
class FileSystem;
class SessionStore;
struct ZoneConfig;

/**
 * @class SystemManager
 * @brief 管理所有子系统的生命周期，分发请求
 */
class SystemManager {
public:
    SystemManager();
    ~SystemManager();

    /// 初始化所有 System
    bool Init(const ZoneConfig& config);

    /// 关闭所有 System
    void Shutdown();

    /// 获取各个 System（用于 Handler 分发请求）
    AuthSystem* GetAuthSystem() { return auth_system_.get(); }
    ChatSystem* GetChatSystem() { return chat_system_.get(); }
    FriendSystem* GetFriendSystem() { return friend_system_.get(); }
    GroupSystem* GetGroupSystem() { return group_system_.get(); }
    FileSystem* GetFileSystem() { return file_system_.get(); }

    /// 获取 SessionStore（用于消息路由）
    std::shared_ptr<SessionStore> GetSessionStore() { return session_store_; }

private:
    std::shared_ptr<SessionStore> session_store_;
    
    // 各子系统实例
    std::unique_ptr<AuthSystem> auth_system_;
    std::unique_ptr<ChatSystem> chat_system_;
    std::unique_ptr<FriendSystem> friend_system_;
    std::unique_ptr<GroupSystem> group_system_;
    std::unique_ptr<FileSystem> file_system_;
};

}  // namespace zone
}  // namespace swift
