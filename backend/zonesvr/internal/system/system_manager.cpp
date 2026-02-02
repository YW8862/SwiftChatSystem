/**
 * @file system_manager.cpp
 * @brief SystemManager 实现
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
    session_store_ = std::make_shared<SessionStore>();
    // if (!session_store_->Init(config)) {
    //     return false;
    // }

    // 2. 创建所有 System
    auth_system_ = std::make_unique<AuthSystem>();
    chat_system_ = std::make_unique<ChatSystem>();
    friend_system_ = std::make_unique<FriendSystem>();
    group_system_ = std::make_unique<GroupSystem>();
    file_system_ = std::make_unique<FileSystem>();

    // 3. 注入 SessionStore
    auth_system_->SetSessionStore(session_store_);
    chat_system_->SetSessionStore(session_store_);
    friend_system_->SetSessionStore(session_store_);
    group_system_->SetSessionStore(session_store_);
    file_system_->SetSessionStore(session_store_);

    // 4. 初始化所有 System（建立 RPC 连接）
    if (!auth_system_->Init()) {
        // LOG_ERROR("Failed to init AuthSystem");
        return false;
    }
    if (!chat_system_->Init()) {
        // LOG_ERROR("Failed to init ChatSystem");
        return false;
    }
    if (!friend_system_->Init()) {
        // LOG_ERROR("Failed to init FriendSystem");
        return false;
    }
    if (!group_system_->Init()) {
        // LOG_ERROR("Failed to init GroupSystem");
        return false;
    }
    if (!file_system_->Init()) {
        // LOG_ERROR("Failed to init FileSystem");
        return false;
    }

    // LOG_INFO("All systems initialized");
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
