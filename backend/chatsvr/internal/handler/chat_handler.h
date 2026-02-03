#pragma once

#include <memory>

namespace swift::chat {

class ChatService;

/**
 * 对外 API 层（Handler）
 * 直接实现 proto 定义的 ChatService gRPC 接口，无独立 API 层。
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
