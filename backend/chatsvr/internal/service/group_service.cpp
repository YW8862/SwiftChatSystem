/**
 * @file group_service.cpp
 * @brief 群组业务逻辑：创建群至少 3 人，邀请仅对非成员生效
 */

#include "group_service.h"
#include "swift/utils.h"
#include <algorithm>
#include <set>
#include <unordered_set>

namespace swift::group_ {

namespace {
constexpr int32_t ROLE_OWNER = 0;
constexpr int32_t ROLE_MEMBER = 1;
constexpr int32_t ROLE_ADMIN = 2;
constexpr int MIN_GROUP_MEMBERS = 3;  // 创建群至少 3 人（含创建者）
}  // namespace

GroupService::GroupService(std::shared_ptr<GroupStore> store) : store_(std::move(store)) {}

GroupService::~GroupService() = default;

CreateGroupResult GroupService::CreateGroup(const std::string& creator_id,
                                              const std::string& group_name,
                                              const std::string& avatar_url,
                                              const std::vector<std::string>& member_ids) {
    CreateGroupResult result;
    if (creator_id.empty()) {
        result.error_code = swift::ErrorCode::INVALID_PARAM;
        return result;
    }

    // 去重：创建者 + 邀请的成员，至少 3 人
    std::set<std::string> unique;
    unique.insert(creator_id);
    for (const auto& id : member_ids) {
        if (!id.empty())
            unique.insert(id);
    }
    if (unique.size() < static_cast<size_t>(MIN_GROUP_MEMBERS)) {
        result.error_code = swift::ErrorCode::GROUP_MEMBERS_TOO_FEW;
        return result;
    }

    std::string group_id = swift::utils::GenerateShortId("g_", 12);
    int64_t now = swift::utils::GetTimestampMs();

    GroupData data;
    data.group_id = group_id;
    data.group_name = group_name.empty() ? "群聊" : group_name;
    data.avatar_url = avatar_url;
    data.owner_id = creator_id;
    data.member_count = static_cast<int32_t>(unique.size());
    data.announcement = "";
    data.created_at = now;
    data.updated_at = now;

    if (!store_->CreateGroup(data)) {
        result.error_code = swift::ErrorCode::INTERNAL_ERROR;
        return result;
    }

    // 先加创建者（群主）
    GroupMemberData owner;
    owner.user_id = creator_id;
    owner.role = ROLE_OWNER;
    owner.nickname = "";
    owner.joined_at = now;
    if (!store_->AddMember(group_id, owner)) {
        store_->DeleteGroup(group_id);
        result.error_code = swift::ErrorCode::INTERNAL_ERROR;
        return result;
    }

    // 再加其余成员
    for (const auto& uid : unique) {
        if (uid == creator_id)
            continue;
        GroupMemberData m;
        m.user_id = uid;
        m.role = ROLE_MEMBER;
        m.nickname = "";
        m.joined_at = now;
        if (!store_->AddMember(group_id, m)) {
            // 已存在或失败，尽量继续
        }
    }

    result.error_code = swift::ErrorCode::OK;
    result.group_id = group_id;
    return result;
}

swift::ErrorCode GroupService::DismissGroup(const std::string& group_id,
                                            const std::string& operator_id) {
    if (group_id.empty() || operator_id.empty())
        return swift::ErrorCode::INVALID_PARAM;

    auto g = store_->GetGroup(group_id);
    if (!g)
        return swift::ErrorCode::GROUP_NOT_FOUND;
    if (g->owner_id != operator_id)
        return swift::ErrorCode::NOT_GROUP_OWNER;

    return store_->DeleteGroup(group_id) ? swift::ErrorCode::OK : swift::ErrorCode::INTERNAL_ERROR;
}

std::optional<GroupData> GroupService::GetGroupInfo(const std::string& group_id) {
    if (group_id.empty())
        return std::nullopt;
    return store_->GetGroup(group_id);
}

swift::ErrorCode GroupService::UpdateGroup(const std::string& group_id,
                                           const std::string& operator_id,
                                           const std::string& group_name,
                                           const std::string& avatar_url,
                                           const std::string& announcement) {
    if (group_id.empty() || operator_id.empty())
        return swift::ErrorCode::INVALID_PARAM;

    auto g = store_->GetGroup(group_id);
    if (!g)
        return swift::ErrorCode::GROUP_NOT_FOUND;
    if (g->owner_id != operator_id) {
        auto mem = store_->GetMember(group_id, operator_id);
        if (!mem || mem->role != ROLE_ADMIN)
            return swift::ErrorCode::PERMISSION_DENIED;
    }

    int64_t now = swift::utils::GetTimestampMs();
    if (!group_name.empty()) g->group_name = group_name;
    if (!avatar_url.empty()) g->avatar_url = avatar_url;
    g->announcement = announcement;
    g->updated_at = now;

    if (!store_->UpdateGroup(group_id, g->group_name, g->avatar_url, g->announcement, now))
        return swift::ErrorCode::INTERNAL_ERROR;
    return swift::ErrorCode::OK;
}

swift::ErrorCode GroupService::InviteMembers(const std::string& group_id,
                                              const std::string& inviter_id,
                                              const std::vector<std::string>& member_ids) {
    if (group_id.empty() || inviter_id.empty())
        return swift::ErrorCode::INVALID_PARAM;

    auto g = store_->GetGroup(group_id);
    if (!g)
        return swift::ErrorCode::GROUP_NOT_FOUND;
    if (!store_->IsMember(group_id, inviter_id))
        return swift::ErrorCode::NOT_GROUP_MEMBER;

    int64_t now = swift::utils::GetTimestampMs();
    for (const auto& uid : member_ids) {
        if (uid.empty())
            continue;
        if (store_->IsMember(group_id, uid))
            continue;  // 已是成员，跳过
        GroupMemberData m;
        m.user_id = uid;
        m.role = ROLE_MEMBER;
        m.nickname = "";
        m.joined_at = now;
        store_->AddMember(group_id, m);
    }
    return swift::ErrorCode::OK;
}

swift::ErrorCode GroupService::RemoveMember(const std::string& group_id,
                                              const std::string& operator_id,
                                              const std::string& member_id) {
    if (group_id.empty() || operator_id.empty() || member_id.empty())
        return swift::ErrorCode::INVALID_PARAM;

    auto g = store_->GetGroup(group_id);
    if (!g)
        return swift::ErrorCode::GROUP_NOT_FOUND;
    if (!store_->IsMember(group_id, member_id))
        return swift::ErrorCode::NOT_GROUP_MEMBER;

    auto op = store_->GetMember(group_id, operator_id);
    if (!op)
        return swift::ErrorCode::NOT_GROUP_MEMBER;
    auto target = store_->GetMember(group_id, member_id);
    if (!target)
        return swift::ErrorCode::NOT_GROUP_MEMBER;

    if (target->role == ROLE_OWNER)
        return swift::ErrorCode::KICK_NOT_ALLOWED;
    if (op->role != ROLE_OWNER && op->role != ROLE_ADMIN)
        return swift::ErrorCode::PERMISSION_DENIED;
    if (op->role == ROLE_ADMIN && target->role == ROLE_ADMIN)
        return swift::ErrorCode::KICK_NOT_ALLOWED;

    return store_->RemoveMember(group_id, member_id) ? swift::ErrorCode::OK
                                                     : swift::ErrorCode::INTERNAL_ERROR;
}

swift::ErrorCode GroupService::LeaveGroup(const std::string& group_id,
                                           const std::string& user_id) {
    if (group_id.empty() || user_id.empty())
        return swift::ErrorCode::INVALID_PARAM;

    auto g = store_->GetGroup(group_id);
    if (!g)
        return swift::ErrorCode::GROUP_NOT_FOUND;
    if (g->owner_id == user_id)
        return swift::ErrorCode::OWNER_CANNOT_LEAVE;
    if (!store_->IsMember(group_id, user_id))
        return swift::ErrorCode::NOT_GROUP_MEMBER;

    return store_->RemoveMember(group_id, user_id) ? swift::ErrorCode::OK
                                                   : swift::ErrorCode::INTERNAL_ERROR;
}

std::vector<GroupMemberData> GroupService::GetGroupMembers(const std::string& group_id,
                                                            int page, int page_size,
                                                            int* total_out) {
    if (group_id.empty())
        return {};
    return store_->GetMembers(group_id, page, page_size, total_out);
}

swift::ErrorCode GroupService::TransferOwner(const std::string& group_id,
                                              const std::string& old_owner_id,
                                              const std::string& new_owner_id) {
    if (group_id.empty() || old_owner_id.empty() || new_owner_id.empty())
        return swift::ErrorCode::INVALID_PARAM;

    auto g = store_->GetGroup(group_id);
    if (!g)
        return swift::ErrorCode::GROUP_NOT_FOUND;
    if (g->owner_id != old_owner_id)
        return swift::ErrorCode::NOT_GROUP_OWNER;
    if (!store_->IsMember(group_id, new_owner_id))
        return swift::ErrorCode::NOT_GROUP_MEMBER;

    if (!store_->UpdateMemberRole(group_id, old_owner_id, ROLE_MEMBER))
        return swift::ErrorCode::INTERNAL_ERROR;
    if (!store_->UpdateMemberRole(group_id, new_owner_id, ROLE_OWNER))
        return swift::ErrorCode::INTERNAL_ERROR;
    if (!store_->UpdateGroupOwner(group_id, new_owner_id))
        return swift::ErrorCode::INTERNAL_ERROR;
    return swift::ErrorCode::OK;
}

swift::ErrorCode GroupService::SetMemberRole(const std::string& group_id,
                                              const std::string& operator_id,
                                              const std::string& member_id,
                                              int32_t role) {
    if (group_id.empty() || operator_id.empty() || member_id.empty())
        return swift::ErrorCode::INVALID_PARAM;

    auto g = store_->GetGroup(group_id);
    if (!g)
        return swift::ErrorCode::GROUP_NOT_FOUND;
    if (g->owner_id != operator_id)
        return swift::ErrorCode::NOT_GROUP_OWNER;

    auto target = store_->GetMember(group_id, member_id);
    if (!target)
        return swift::ErrorCode::NOT_GROUP_MEMBER;
    if (target->role == ROLE_OWNER)
        return swift::ErrorCode::PERMISSION_DENIED;  // 不能改群主角色

    return store_->UpdateMemberRole(group_id, member_id, role) ? swift::ErrorCode::OK
                                                               : swift::ErrorCode::INTERNAL_ERROR;
}

std::vector<GroupData> GroupService::GetUserGroups(const std::string& user_id) {
    std::vector<GroupData> result;
    if (user_id.empty())
        return result;

    auto ids = store_->GetUserGroupIds(user_id);
    for (const auto& gid : ids) {
        auto g = store_->GetGroup(gid);
        if (g)
            result.push_back(std::move(*g));
    }
    return result;
}

}  // namespace swift::group_
