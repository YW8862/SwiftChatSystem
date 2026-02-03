/**
 * @file friend_service.cpp
 * @brief 好友业务逻辑：好友请求、好友关系、分组、黑名单
 */

#include "friend_service.h"
#include "swift/utils.h"
#include <algorithm>

namespace swift::friend_ {

FriendService::FriendService(std::shared_ptr<FriendStore> store)
    : store_(std::move(store)) {}

FriendService::~FriendService() = default;

// ============================================================================
// 好友请求
// ============================================================================

bool FriendService::AddFriend(const std::string& user_id, const std::string& friend_id,
                              const std::string& remark) {
    if (user_id.empty() || friend_id.empty())
        return false;
    if (user_id == friend_id)
        return false;  // 不能加自己为好友
    if (store_->IsFriend(user_id, friend_id))
        return false;  // 已是好友
    if (store_->IsBlocked(friend_id, user_id))
        return false;  // 被对方拉黑，无法发送请求

    // 是否已有待处理请求（同一对用户只保留一条待处理）
    auto received = store_->GetReceivedRequests(friend_id);
    for (const auto& r : received) {
        if (r.from_user_id == user_id && r.status == 0)
            return false;  // 已有待处理请求
    }

    FriendRequestData req;
    req.request_id = swift::utils::GenerateShortId("req_", 12);
    req.from_user_id = user_id;
    req.to_user_id = friend_id;
    req.remark = remark;
    req.status = 0;  // 待处理
    req.created_at = swift::utils::GetTimestampMs();

    return store_->CreateRequest(req);
}

bool FriendService::HandleRequest(const std::string& user_id, const std::string& request_id,
                                  bool accept, const std::string& group_id) {
    if (user_id.empty() || request_id.empty())
        return false;

    auto req = store_->GetRequest(request_id);
    if (!req)
        return false;
    if (req->to_user_id != user_id)
        return false;  // 只有接收方可以处理
    if (req->status != 0)
        return false;  // 已处理过

    if (accept) {
        // 接收方(user_id) 添加 发送方(from_user_id) 为好友，放入 group_id；空则放入默认分组
        std::string target_group = group_id.empty() ? kDefaultFriendGroupId : group_id;
        EnsureDefaultGroup(user_id);

        int64_t now = swift::utils::GetTimestampMs();
        FriendData data;
        data.user_id = user_id;
        data.friend_id = req->from_user_id;
        data.remark = "";
        data.group_id = target_group;
        data.added_at = now;
        if (!store_->AddFriend(data))
            return false;  // 可能已互为好友或其它错误
        return store_->UpdateRequestStatus(request_id, 1);  // 已接受
    } else {
        return store_->UpdateRequestStatus(request_id, 2);  // 已拒绝
    }
}

// ============================================================================
// 好友关系
// ============================================================================

bool FriendService::RemoveFriend(const std::string& user_id, const std::string& friend_id) {
    if (user_id.empty() || friend_id.empty())
        return false;
    if (!store_->IsFriend(user_id, friend_id))
        return false;
    return store_->RemoveFriend(user_id, friend_id);
}

std::vector<FriendData> FriendService::GetFriends(const std::string& user_id,
                                                  const std::string& group_id) {
    if (user_id.empty())
        return {};
    return store_->GetFriends(user_id, group_id);
}

// ============================================================================
// 黑名单
// ============================================================================

bool FriendService::Block(const std::string& user_id, const std::string& target_id) {
    if (user_id.empty() || target_id.empty())
        return false;
    if (user_id == target_id)
        return false;
    return store_->Block(user_id, target_id);
}

bool FriendService::Unblock(const std::string& user_id, const std::string& target_id) {
    if (user_id.empty() || target_id.empty())
        return false;
    return store_->Unblock(user_id, target_id);
}

std::vector<std::string> FriendService::GetBlockList(const std::string& user_id) {
    if (user_id.empty())
        return {};
    return store_->GetBlockList(user_id);
}

// ============================================================================
// 分组
// ============================================================================

bool FriendService::CreateFriendGroup(const std::string& user_id,
                                      const std::string& group_name,
                                      std::string* out_group_id) {
    if (user_id.empty() || group_name.empty())
        return false;

    std::string group_id = swift::utils::GenerateShortId("g_", 8);
    FriendGroupData g;
    g.group_id = group_id;
    g.user_id = user_id;
    g.group_name = group_name;
    g.sort_order = 0;

    if (!store_->CreateGroup(g))
        return false;
    if (out_group_id)
        *out_group_id = group_id;
    return true;
}

std::vector<FriendGroupData> FriendService::GetFriendGroups(const std::string& user_id) {
    if (user_id.empty())
        return {};
    EnsureDefaultGroup(user_id);
    return store_->GetGroups(user_id);
}

swift::ErrorCode FriendService::DeleteFriendGroup(const std::string& user_id,
                                                   const std::string& group_id) {
    if (user_id.empty())
        return swift::ErrorCode::INVALID_PARAM;
    // 默认分组不能删除（空或 "default" 均视为默认分组）
    if (group_id.empty() || group_id == kDefaultFriendGroupId)
        return swift::ErrorCode::FRIEND_GROUP_DEFAULT;

    auto groups = store_->GetGroups(user_id);
    bool found = false;
    for (const auto& g : groups) {
        if (g.group_id == group_id) {
            found = true;
            break;
        }
    }
    if (!found)
        return swift::ErrorCode::FRIEND_GROUP_NOT_FOUND;

    // Store 已实现：删除分组时将该分组下所有好友的 group_id 置为 ""（默认分组）
    if (!store_->DeleteGroup(user_id, group_id))
        return swift::ErrorCode::INTERNAL_ERROR;
    return swift::ErrorCode::OK;
}

bool FriendService::MoveFriend(const std::string& user_id, const std::string& friend_id,
                                const std::string& to_group_id) {
    if (user_id.empty() || friend_id.empty())
        return false;
    if (!store_->IsFriend(user_id, friend_id))
        return false;
    return store_->MoveFriend(user_id, friend_id, to_group_id);
}

bool FriendService::SetRemark(const std::string& user_id, const std::string& friend_id,
                               const std::string& remark) {
    if (user_id.empty() || friend_id.empty())
        return false;
    if (!store_->IsFriend(user_id, friend_id))
        return false;
    return store_->UpdateRemark(user_id, friend_id, remark);
}

void FriendService::EnsureDefaultGroup(const std::string& user_id) {
    if (user_id.empty())
        return;
    auto groups = store_->GetGroups(user_id);
    for (const auto& g : groups) {
        if (g.group_id == kDefaultFriendGroupId)
            return;
    }
    FriendGroupData def;
    def.group_id = kDefaultFriendGroupId;
    def.user_id = user_id;
    def.group_name = kDefaultFriendGroupName;
    def.sort_order = 0;
    store_->CreateGroup(def);
}

// ============================================================================
// 好友请求列表
// ============================================================================

std::vector<FriendRequestData> FriendService::GetFriendRequests(const std::string& user_id,
                                                                 int type) {
    if (user_id.empty())
        return {};

    if (type == 1) {
        return store_->GetReceivedRequests(user_id);
    }
    if (type == 2) {
        return store_->GetSentRequests(user_id);
    }
    // type == 0: 全部，先收到的后发出的
    auto received = store_->GetReceivedRequests(user_id);
    auto sent = store_->GetSentRequests(user_id);
    std::vector<FriendRequestData> result;
    result.reserve(received.size() + sent.size());
    for (auto& r : received)
        result.push_back(std::move(r));
    for (auto& s : sent)
        result.push_back(std::move(s));
    return result;
}

}  // namespace swift::friend_
