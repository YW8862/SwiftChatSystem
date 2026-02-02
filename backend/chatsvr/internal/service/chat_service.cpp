#include "chat_service.h"

namespace swift::chat {

ChatService::ChatService(std::shared_ptr<MessageStore> msg_store,
                         std::shared_ptr<ConversationStore> conv_store)
    : msg_store_(std::move(msg_store))
    , conv_store_(std::move(conv_store)) {}

ChatService::~ChatService() = default;

// TODO: 实现各业务方法

}  // namespace swift::chat
