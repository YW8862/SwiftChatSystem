#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "../store/group_store.h"
#include "swift/error_code.h"

namespace swift::group_ {

/**
 * 创建群结果：成功返回 group_id，失败返回错误码
 */
struct CreateGroupResult {
    swift::ErrorCode error_code = swift::ErrorCode::OK;
    std::string group_id;
};

/**
 * 群组业务逻辑
 * - 创建群：至少 3 人（创建者 + 邀请的成员），不允许 1 人或 2 人建群
 * - 邀请成员：仅对当前不在群内的用户生效
 */
class GroupService {
public:
    explicit GroupService(std::shared_ptr<GroupStore> store);
    ~GroupService();

    CreateGroupResult CreateGroup(const std::string& creator_id,
                                  const std::string& group_name,
                                  const std::string& avatar_url,
                                  const std::vector<std::string>& member_ids);

    swift::ErrorCode DismissGroup(const std::string& group_id, const std::string& operator_id);

    // 成功返回群信息，失败返回错误码（通过 optional + 错误码约定或 Result）
    std::optional<GroupData> GetGroupInfo(const std::string& group_id);

    swift::ErrorCode UpdateGroup(const std::string& group_id, const std::string& operator_id,
                                  const std::string& group_name, const std::string& avatar_url,
                                  const std::string& announcement);

    // 邀请成员：仅添加当前不在群内的用户
    swift::ErrorCode InviteMembers(const std::string& group_id, const std::string& inviter_id,
                                    const std::vector<std::string>& member_ids);

    swift::ErrorCode RemoveMember(const std::string& group_id, const std::string& operator_id,
                                   const std::string& member_id);

    swift::ErrorCode LeaveGroup(const std::string& group_id, const std::string& user_id);

    std::vector<GroupMemberData> GetGroupMembers(const std::string& group_id,
                                                   int page, int page_size, int* total_out);

    swift::ErrorCode TransferOwner(const std::string& group_id, const std::string& old_owner_id,
                                     const std::string& new_owner_id);

    swift::ErrorCode SetMemberRole(const std::string& group_id, const std::string& operator_id,
                                    const std::string& member_id, int32_t role);

    std::vector<GroupData> GetUserGroups(const std::string& user_id);

private:
    std::shared_ptr<GroupStore> store_;
};

}  // namespace swift::group_
