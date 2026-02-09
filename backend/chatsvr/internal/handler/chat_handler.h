#pragma once

#include <memory>
#include "chat.grpc.pb.h"

namespace swift::chat {

class ChatServiceCore;  // 业务逻辑类，与 proto 生成的 ChatService 区分

/**
 * 对外 API 层（Handler）
 * 直接实现 proto 定义的 ChatService gRPC 接口。
 */
class ChatHandler : public ::swift::chat::ChatService::Service {
public:
    /** @param jwt_secret 与 OnlineSvr 一致，用于从 metadata 校验 Token，得到当前用户 id */
    ChatHandler(std::shared_ptr<ChatServiceCore> service, const std::string& jwt_secret);
    ~ChatHandler() override;

    ::grpc::Status SendMessage(::grpc::ServerContext* context,
                               const ::swift::chat::SendMessageRequest* request,
                               ::swift::chat::SendMessageResponse* response) override;

    ::grpc::Status RecallMessage(::grpc::ServerContext* context,
                                 const ::swift::chat::RecallMessageRequest* request,
                                 ::swift::chat::RecallMessageResponse* response) override;

    ::grpc::Status PullOffline(::grpc::ServerContext* context,
                               const ::swift::chat::PullOfflineRequest* request,
                               ::swift::chat::PullOfflineResponse* response) override;

    ::grpc::Status MarkRead(::grpc::ServerContext* context,
                            const ::swift::chat::MarkReadRequest* request,
                            ::swift::chat::MarkReadResponse* response) override;

    ::grpc::Status GetHistory(::grpc::ServerContext* context,
                              const ::swift::chat::GetHistoryRequest* request,
                              ::swift::chat::GetHistoryResponse* response) override;

    ::grpc::Status SyncConversations(::grpc::ServerContext* context,
                                     const ::swift::chat::SyncConversationsRequest* request,
                                     ::swift::chat::SyncConversationsResponse* response) override;

    ::grpc::Status DeleteConversation(::grpc::ServerContext* context,
                                      const ::swift::chat::DeleteConversationRequest* request,
                                      ::swift::chat::DeleteConversationResponse* response) override;

private:
    std::shared_ptr<ChatServiceCore> service_;
    std::string jwt_secret_;
};

}  // namespace swift::chat
