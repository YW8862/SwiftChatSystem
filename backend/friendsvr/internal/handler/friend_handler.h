#pragma once

#include <memory>

namespace swift::friend_ {

class FriendService;

class FriendHandler {
public:
    explicit FriendHandler(std::shared_ptr<FriendService> service);
    ~FriendHandler();
    
private:
    std::shared_ptr<FriendService> service_;
};

}  // namespace swift::friend_
