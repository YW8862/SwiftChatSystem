/**
 * @file rpc_client_base.cpp
 * @brief RpcClientBase 实现
 */

#include "rpc_client_base.h"
#include <chrono>

namespace swift {
namespace zone {

bool RpcClientBase::Connect(const std::string& address) {
    address_ = address;
    
    // 创建 Channel
    grpc::ChannelArguments args;
    args.SetMaxReceiveMessageSize(64 * 1024 * 1024);  // 64MB
    args.SetMaxSendMessageSize(64 * 1024 * 1024);
    
    channel_ = grpc::CreateCustomChannel(
        address,
        grpc::InsecureChannelCredentials(),
        args
    );
    
    // 等待连接就绪
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

std::unique_ptr<grpc::ClientContext> RpcClientBase::CreateContext(int timeout_ms) {
    auto context = std::make_unique<grpc::ClientContext>();
    auto deadline = std::chrono::system_clock::now() + 
                    std::chrono::milliseconds(timeout_ms);
    context->set_deadline(deadline);
    return context;
}

}  // namespace zone
}  // namespace swift
