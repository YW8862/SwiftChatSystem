/**
 * @file group_system.h
 * @brief 群组子系统
 * 
 * GroupSystem 负责处理群组相关的请求：
 * - 创建/解散群组
 * - 成员管理（邀请/移除/退出）
 * - 群信息管理
 * - 群成员角色管理
 * 
 * 内部调用 ChatSvr (GroupService) 的 gRPC 接口完成实际业务。
 */

#pragma once

#include "base_system.h"
#include "../rpc/group_rpc_client.h"
#include <memory>
#include <string>
#include <vector>

namespace swift {
namespace zone {

class GroupRpcClient;

/**
 * @class GroupSystem
 * @brief 群组子系统
 */
class GroupSystem : public BaseSystem {
public:
    GroupSystem();
    ~GroupSystem() override;

    std::string Name() const override { return "GroupSystem"; }
    bool Init() override;
    void Shutdown() override;

    // ============ 业务接口 ============

    /// 创建群组
    /// @return 群组ID，失败返回空字符串
    std::string CreateGroup(const std::string& creator_id,
                           const std::string& group_name,
                           const std::vector<std::string>& member_ids);

    /// 解散群组
    bool DismissGroup(const std::string& group_id, const std::string& operator_id);

    /// 邀请成员
    bool InviteMembers(const std::string& group_id,
                       const std::string& inviter_id,
                       const std::vector<std::string>& member_ids);

    /// 移除成员
    bool RemoveMember(const std::string& group_id,
                      const std::string& operator_id,
                      const std::string& member_id);

    /// 退出群组
    bool LeaveGroup(const std::string& group_id, const std::string& user_id);

    /// 转让群主
    bool TransferOwner(const std::string& group_id,
                       const std::string& old_owner_id,
                       const std::string& new_owner_id);

    /// 获取群成员列表（用于消息推送给在线成员）
    bool GetGroupMembers(const std::string& group_id, int32_t page, int32_t page_size,
                         std::vector<GroupMemberResult>* out_members, int32_t* total,
                         std::string* out_error);

    /// 获取群信息
    bool GetGroupInfo(const std::string& group_id, GroupInfoResult* out_info, std::string* out_error);

    /// 获取用户加入的群列表
    bool GetUserGroups(const std::string& user_id, std::vector<GroupInfoResult>* out_groups,
                       std::string* out_error);

private:
    /// 广播群消息给所有在线成员
    void BroadcastToGroupMembers(const std::string& group_id, const std::string& payload);

private:
    std::unique_ptr<GroupRpcClient> rpc_client_;
};

}  // namespace zone
}  // namespace swift
