/**
 * @file system_manager.cpp
 * @brief SystemManager 实现：创建 SessionStore、各 System，注入配置并初始化
 */

#include "system_manager.h"
#include "auth_system.h"
#include "chat_system.h"
#include "friend_system.h"
#include "group_system.h"
#include "file_system.h"
#include "../store/session_store.h"
#include "../config/config.h"

namespace swift {
namespace zone {

SystemManager::SystemManager() = default;

SystemManager::~SystemManager() {
    Shutdown();
}

bool SystemManager::Init(const ZoneConfig& config) {
    // 1. 创建 SessionStore（所有 System 共享）
    if (config.session_store_type == "redis") {
        session_store_ = std::make_shared<RedisSessionStore>(config.redis_url);
    } else {
        session_store_ = std::make_shared<MemorySessionStore>();
    }

    // 2. 创建所有 System
    auth_system_ = std::make_unique<AuthSystem>();
    chat_system_ = std::make_unique<ChatSystem>();
    friend_system_ = std::make_unique<FriendSystem>();
    group_system_ = std::make_unique<GroupSystem>();
    file_system_ = std::make_unique<FileSystem>();

    // 3. 注入 SessionStore 与配置
    for (BaseSystem* sys : {static_cast<BaseSystem*>(auth_system_.get()),
                            static_cast<BaseSystem*>(chat_system_.get()),
                            static_cast<BaseSystem*>(friend_system_.get()),
                            static_cast<BaseSystem*>(group_system_.get()),
                            static_cast<BaseSystem*>(file_system_.get())}) {
        sys->SetSessionStore(session_store_);
        sys->SetConfig(&config);
    }

    // 4. 初始化所有 System（建立 RPC 连接）
    if (!auth_system_->Init()) return false;
    if (!chat_system_->Init()) return false;
    if (!friend_system_->Init()) return false;
    if (!group_system_->Init()) return false;
    if (!file_system_->Init()) return false;

    return true;
}

void SystemManager::Shutdown() {
    // 关闭所有 System
    if (file_system_) {
        file_system_->Shutdown();
        file_system_.reset();
    }
    if (group_system_) {
        group_system_->Shutdown();
        group_system_.reset();
    }
    if (friend_system_) {
        friend_system_->Shutdown();
        friend_system_.reset();
    }
    if (chat_system_) {
        chat_system_->Shutdown();
        chat_system_.reset();
    }
    if (auth_system_) {
        auth_system_->Shutdown();
        auth_system_.reset();
    }

    if (session_store_) {
        // session_store_->Shutdown();
        session_store_.reset();
    }
}

}  // namespace zone
}  // namespace swift
