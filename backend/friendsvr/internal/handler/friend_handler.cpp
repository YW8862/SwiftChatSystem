#include "friend_handler.h"

namespace swift::friend_ {

FriendHandler::FriendHandler(std::shared_ptr<FriendService> service)
    : service_(std::move(service)) {}

FriendHandler::~FriendHandler() = default;

}  // namespace swift::friend_
