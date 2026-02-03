#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../store/friend_store.h"
#include "swift/error_code.h"

namespace swift::friend_ {

class FriendService {
public:
    explicit FriendService(std::shared_ptr<FriendStore> store);
    ~FriendService();

    // 发送好友请求（不直接加好友，需对方处理）
    bool AddFriend(const std::string& user_id, const std::string& friend_id,
                   const std::string& remark);

    // 处理好友请求（仅接收方可处理）：accept 为 true 时加为好友并放入 group_id
    bool HandleRequest(const std::string& user_id, const std::string& request_id,
                       bool accept, const std::string& group_id);

    // 删除好友（双向）
    bool RemoveFriend(const std::string& user_id, const std::string& friend_id);

    // 获取好友列表，group_id 为空表示全部
    std::vector<FriendData> GetFriends(const std::string& user_id,
                                       const std::string& group_id = "");

    // 拉黑 / 取消拉黑
    bool Block(const std::string& user_id, const std::string& target_id);
    bool Unblock(const std::string& user_id, const std::string& target_id);

    // 获取黑名单
    std::vector<std::string> GetBlockList(const std::string& user_id);

    // 创建好友分组（自动生成 group_id）
    bool CreateFriendGroup(const std::string& user_id, const std::string& group_name,
                           std::string* out_group_id = nullptr);

    // 获取全部分组
    std::vector<FriendGroupData> GetFriendGroups(const std::string& user_id);

    // 删除好友分组（分组内好友移至默认分组；默认分组不能删除）
    swift::ErrorCode DeleteFriendGroup(const std::string& user_id, const std::string& group_id);

    // 移动好友到指定分组
    bool MoveFriend(const std::string& user_id, const std::string& friend_id,
                    const std::string& to_group_id);

    // 设置好友备注
    bool SetRemark(const std::string& user_id, const std::string& friend_id,
                   const std::string& remark);

    // 获取好友请求列表：type 0=全部, 1=收到的, 2=发出的
    std::vector<FriendRequestData> GetFriendRequests(const std::string& user_id, int type = 0);

private:
    /** 确保默认分组「我的好友」存在（不存在则创建） */
    void EnsureDefaultGroup(const std::string& user_id);

    std::shared_ptr<FriendStore> store_;
};

}  // namespace swift::friend_
