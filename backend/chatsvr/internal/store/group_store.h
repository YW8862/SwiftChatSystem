#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace swift::group_ {

/**
 * 群组元数据（与 proto GroupInfo 对应）
 */
struct GroupData {
    std::string group_id;
    std::string group_name;
    std::string avatar_url;
    std::string owner_id;
    int32_t member_count = 0;
    std::string announcement;
    int64_t created_at = 0;
    int64_t updated_at = 0;
    int32_t status = 0;  // 0=正常, 1=已解散（会话结束，群 ID 不可复用）
};

/**
 * 群成员（与 proto GroupMember 对应，不含 profile）
 */
struct GroupMemberData {
    std::string user_id;
    int32_t role = 1;   // 0=群主, 1=普通成员, 2=管理员
    std::string nickname;
    int64_t joined_at = 0;
};

/**
 * 群组存储接口
 *
 * RocksDB Key 设计：
 *   group:{group_id}                 -> GroupData JSON
 *   group_member:{group_id}:{user_id} -> GroupMemberData JSON
 *   user_groups:{user_id}:{group_id}  -> "" (用户加入的群索引)
 */
class GroupStore {
public:
    virtual ~GroupStore() = default;

    // === 群组 ===
    virtual bool CreateGroup(const GroupData& data) = 0;
    virtual std::optional<GroupData> GetGroup(const std::string& group_id) = 0;
    virtual bool UpdateGroup(const std::string& group_id, const std::string& group_name,
                             const std::string& avatar_url, const std::string& announcement,
                             int64_t updated_at = 0) = 0;
    virtual bool UpdateGroupOwner(const std::string& group_id, const std::string& new_owner_id) = 0;
    virtual bool DeleteGroup(const std::string& group_id) = 0;
    /** 群主解散群：status=1，移除所有成员索引，群 ID 保留不可复用；消息仍存服务器 */
    virtual bool DissolveGroup(const std::string& group_id) = 0;

    // === 成员 ===
    virtual bool AddMember(const std::string& group_id, const GroupMemberData& member) = 0;
    virtual bool RemoveMember(const std::string& group_id, const std::string& user_id) = 0;
    virtual std::optional<GroupMemberData> GetMember(const std::string& group_id,
                                                       const std::string& user_id) = 0;
    virtual std::vector<GroupMemberData> GetMembers(const std::string& group_id,
                                                     int page, int page_size,
                                                     int* total_out = nullptr) = 0;
    virtual bool UpdateMemberRole(const std::string& group_id, const std::string& user_id,
                                   int32_t role) = 0;
    virtual bool IsMember(const std::string& group_id, const std::string& user_id) = 0;

    // === 用户群列表 ===
    virtual std::vector<std::string> GetUserGroupIds(const std::string& user_id) = 0;
};

/**
 * RocksDB 实现
 */
class RocksDBGroupStore : public GroupStore {
public:
    explicit RocksDBGroupStore(const std::string& db_path);
    ~RocksDBGroupStore() override;

    bool CreateGroup(const GroupData& data) override;
    std::optional<GroupData> GetGroup(const std::string& group_id) override;
    bool UpdateGroup(const std::string& group_id, const std::string& group_name,
                     const std::string& avatar_url, const std::string& announcement,
                     int64_t updated_at = 0) override;
    bool UpdateGroupOwner(const std::string& group_id, const std::string& new_owner_id) override;
    bool DeleteGroup(const std::string& group_id) override;
    bool DissolveGroup(const std::string& group_id) override;

    bool AddMember(const std::string& group_id, const GroupMemberData& member) override;
    bool RemoveMember(const std::string& group_id, const std::string& user_id) override;
    std::optional<GroupMemberData> GetMember(const std::string& group_id,
                                              const std::string& user_id) override;
    std::vector<GroupMemberData> GetMembers(const std::string& group_id,
                                            int page, int page_size,
                                            int* total_out = nullptr) override;
    bool UpdateMemberRole(const std::string& group_id, const std::string& user_id,
                          int32_t role) override;
    bool IsMember(const std::string& group_id, const std::string& user_id) override;

    std::vector<std::string> GetUserGroupIds(const std::string& user_id) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace swift::group_
