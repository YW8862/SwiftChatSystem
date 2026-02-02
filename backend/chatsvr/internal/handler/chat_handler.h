#pragma once

#include <memory>

namespace swift::chat {

class ChatService;

/**
 * ChatService gRPC 处理器
 */
class ChatHandler {
public:
    explicit ChatHandler(std::shared_ptr<ChatService> service);
    ~ChatHandler();
    
    // gRPC 接口
    // Status SendMessage(...);
    // Status RecallMessage(...);
    // Status PullOffline(...);
    // Status SearchMessages(...);
    // Status MarkRead(...);
    // Status GetHistory(...);
    // Status SyncConversations(...);
    
private:
    std::shared_ptr<ChatService> service_;
};

}  // namespace swift::chat
