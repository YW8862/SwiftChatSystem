#include "friend_service.h"

namespace swift::friend_ {

FriendService::FriendService(std::shared_ptr<FriendStore> store)
    : store_(std::move(store)) {}

FriendService::~FriendService() = default;

}  // namespace swift::friend_
