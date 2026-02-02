#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../store/friend_store.h"

namespace swift::friend_ {

class FriendService {
public:
    explicit FriendService(std::shared_ptr<FriendStore> store);
    ~FriendService();
    
    // 发送好友请求
    bool AddFriend(const std::string& user_id, const std::string& friend_id,
                   const std::string& remark);
    
    // 处理好友请求
    bool HandleRequest(const std::string& user_id, const std::string& request_id,
                       bool accept, const std::string& group_id);
    
    // 删除好友
    bool RemoveFriend(const std::string& user_id, const std::string& friend_id);
    
    // 获取好友列表
    std::vector<FriendData> GetFriends(const std::string& user_id,
                                       const std::string& group_id = "");
    
    // 拉黑/取消拉黑
    bool Block(const std::string& user_id, const std::string& target_id);
    bool Unblock(const std::string& user_id, const std::string& target_id);
    
private:
    std::shared_ptr<FriendStore> store_;
};

}  // namespace swift::friend_
