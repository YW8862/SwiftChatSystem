#include "chat_handler.h"

namespace swift::chat {

ChatHandler::ChatHandler(std::shared_ptr<ChatService> service)
    : service_(std::move(service)) {}

ChatHandler::~ChatHandler() = default;

}  // namespace swift::chat
