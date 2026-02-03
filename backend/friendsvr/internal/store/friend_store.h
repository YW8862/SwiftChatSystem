#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace swift::friend_ {

/**
 * 好友关系数据
 */
struct FriendData {
    std::string user_id;
    std::string friend_id;
    std::string remark;          // 备注名
    std::string group_id;        // 分组 ID
    int64_t added_at = 0;
};

/**
 * 好友请求数据
 */
struct FriendRequestData {
    std::string request_id;
    std::string from_user_id;
    std::string to_user_id;
    std::string remark;          // 验证消息
    int status = 0;              // 0=待处理, 1=已接受, 2=已拒绝
    int64_t created_at = 0;
};

/**
 * 好友分组
 */
struct FriendGroupData {
    std::string group_id;
    std::string user_id;
    std::string group_name;
    int sort_order = 0;
};

/**
 * 好友存储接口
 * 
 * RocksDB Key 设计：
 *   friend:{user_id}:{friend_id}           -> FriendData
 *   friend_req:{request_id}                -> FriendRequestData
 *   friend_req_idx:{to_user_id}:{req_id}   -> request_id (收到的请求索引)
 *   friend_group:{user_id}:{group_id}      -> FriendGroupData
 *   block:{user_id}:{target_id}            -> "1" (黑名单)
 */
class FriendStore {
public:
    virtual ~FriendStore() = default;
    
    // === 好友关系 ===
    virtual bool AddFriend(const FriendData& data) = 0;
    virtual bool RemoveFriend(const std::string& user_id, const std::string& friend_id) = 0;
    virtual std::vector<FriendData> GetFriends(const std::string& user_id, 
                                                const std::string& group_id = "") = 0;
    virtual bool IsFriend(const std::string& user_id, const std::string& friend_id) = 0;
    virtual bool UpdateRemark(const std::string& user_id, const std::string& friend_id,
                              const std::string& remark) = 0;
    virtual bool MoveFriend(const std::string& user_id, const std::string& friend_id,
                            const std::string& to_group_id) = 0;
    
    // === 好友请求 ===
    virtual bool CreateRequest(const FriendRequestData& req) = 0;
    virtual std::optional<FriendRequestData> GetRequest(const std::string& request_id) = 0;
    virtual bool UpdateRequestStatus(const std::string& request_id, int status) = 0;
    virtual std::vector<FriendRequestData> GetReceivedRequests(const std::string& user_id) = 0;
    virtual std::vector<FriendRequestData> GetSentRequests(const std::string& user_id) = 0;
    
    // === 分组 ===
    virtual bool CreateGroup(const FriendGroupData& group) = 0;
    virtual std::vector<FriendGroupData> GetGroups(const std::string& user_id) = 0;
    virtual bool DeleteGroup(const std::string& user_id, const std::string& group_id) = 0;
    
    // === 黑名单 ===
    virtual bool Block(const std::string& user_id, const std::string& target_id) = 0;
    virtual bool Unblock(const std::string& user_id, const std::string& target_id) = 0;
    virtual bool IsBlocked(const std::string& user_id, const std::string& target_id) = 0;
    virtual std::vector<std::string> GetBlockList(const std::string& user_id) = 0;
};

/**
 * RocksDB 实现
 */
class RocksDBFriendStore : public FriendStore {
public:
    explicit RocksDBFriendStore(const std::string& db_path);
    ~RocksDBFriendStore() override;

    bool AddFriend(const FriendData& data) override;
    bool RemoveFriend(const std::string& user_id, const std::string& friend_id) override;
    std::vector<FriendData> GetFriends(const std::string& user_id,
                                       const std::string& group_id = "") override;
    bool IsFriend(const std::string& user_id, const std::string& friend_id) override;
    bool UpdateRemark(const std::string& user_id, const std::string& friend_id,
                     const std::string& remark) override;
    bool MoveFriend(const std::string& user_id, const std::string& friend_id,
                    const std::string& to_group_id) override;

    bool CreateRequest(const FriendRequestData& req) override;
    std::optional<FriendRequestData> GetRequest(const std::string& request_id) override;
    bool UpdateRequestStatus(const std::string& request_id, int status) override;
    std::vector<FriendRequestData> GetReceivedRequests(const std::string& user_id) override;
    std::vector<FriendRequestData> GetSentRequests(const std::string& user_id) override;

    bool CreateGroup(const FriendGroupData& group) override;
    std::vector<FriendGroupData> GetGroups(const std::string& user_id) override;
    bool DeleteGroup(const std::string& user_id, const std::string& group_id) override;

    bool Block(const std::string& user_id, const std::string& target_id) override;
    bool Unblock(const std::string& user_id, const std::string& target_id) override;
    bool IsBlocked(const std::string& user_id, const std::string& target_id) override;
    std::vector<std::string> GetBlockList(const std::string& user_id) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace swift::friend_
