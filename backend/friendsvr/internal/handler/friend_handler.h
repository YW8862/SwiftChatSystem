#pragma once

#include <memory>

namespace swift::friend_ {

class FriendService;

/**
 * 对外 API 层（Handler）
 * 直接实现 proto 定义的 FriendService gRPC 接口，无独立 API 层。
 */
class FriendHandler {
public:
    explicit FriendHandler(std::shared_ptr<FriendService> service);
    ~FriendHandler();
    
private:
    std::shared_ptr<FriendService> service_;
};

}  // namespace swift::friend_
