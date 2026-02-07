/**
 * @file friend_system.cpp
 * @brief FriendSystem 实现
 */

#include "friend_system.h"
#include "../rpc/friend_rpc_client.h"
#include "../config/config.h"

namespace swift {
namespace zone {

FriendSystem::FriendSystem() = default;
FriendSystem::~FriendSystem() = default;

bool FriendSystem::Init() {
    if (!config_) return true;
    rpc_client_ = std::make_unique<FriendRpcClient>();
    if (!rpc_client_->Connect(config_->friend_svr_addr)) return false;
    rpc_client_->InitStub();
    return true;
}

void FriendSystem::Shutdown() {
    if (rpc_client_) {
        rpc_client_.reset();
    }
}

bool FriendSystem::AddFriend(const std::string& user_id, 
                              const std::string& friend_id, 
                              const std::string& remark) {
    // TODO: 
    // 1. 调用 FriendSvr.AddFriend
    // 2. 如果目标用户在线，推送好友请求通知
    return false;
}

bool FriendSystem::HandleFriendRequest(const std::string& user_id, 
                                        const std::string& request_id, 
                                        bool accept) {
    // TODO:
    // 1. 调用 FriendSvr.HandleFriendRequest
    // 2. 如果接受，通知双方
    return false;
}

bool FriendSystem::RemoveFriend(const std::string& user_id, const std::string& friend_id) {
    // TODO: 调用 FriendSvr.RemoveFriend
    return false;
}

bool FriendSystem::BlockUser(const std::string& user_id, const std::string& target_id) {
    // TODO: 调用 FriendSvr.BlockUser
    return false;
}

bool FriendSystem::UnblockUser(const std::string& user_id, const std::string& target_id) {
    // TODO: 调用 FriendSvr.UnblockUser
    return false;
}

bool FriendSystem::IsFriend(const std::string& user_id, const std::string& friend_id) {
    // TODO: 调用 FriendSvr 或使用缓存
    return false;
}

bool FriendSystem::IsBlocked(const std::string& user_id, const std::string& target_id) {
    // TODO: 调用 FriendSvr 或使用缓存
    return false;
}

}  // namespace zone
}  // namespace swift
