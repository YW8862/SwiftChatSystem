/**
 * @file rpc_client_base.cpp
 * @brief RpcClientBase 实现
 */

#include "rpc_client_base.h"
#include <chrono>

namespace swift {
namespace zone {

bool RpcClientBase::Connect(const std::string& address, bool wait_ready) {
    address_ = address;

    grpc::ChannelArguments args;
    args.SetMaxReceiveMessageSize(64 * 1024 * 1024);  // 64MB
    args.SetMaxSendMessageSize(64 * 1024 * 1024);

    channel_ = grpc::CreateCustomChannel(
        address,
        grpc::InsecureChannelCredentials(),
        args
    );

    if (!wait_ready) return true;
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
    return channel_->WaitForConnected(deadline);
}

void RpcClientBase::Disconnect() {
    channel_.reset();
    address_.clear();
}

bool RpcClientBase::IsConnected() const {
    if (!channel_) return false;
    auto state = channel_->GetState(false);
    return state == GRPC_CHANNEL_READY || state == GRPC_CHANNEL_IDLE;
}

std::unique_ptr<grpc::ClientContext> RpcClientBase::CreateContext(int timeout_ms,
                                                                   const std::string& token) {
    auto context = std::make_unique<grpc::ClientContext>();
    auto deadline = std::chrono::system_clock::now() +
                    std::chrono::milliseconds(timeout_ms);
    context->set_deadline(deadline);
    if (!token.empty())
        context->AddMetadata("authorization", "Bearer " + token);
    return context;
}

}  // namespace zone
}  // namespace swift
