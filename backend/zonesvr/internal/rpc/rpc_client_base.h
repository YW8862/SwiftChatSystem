/**
 * @file rpc_client_base.h
 * @brief gRPC 客户端基类
 * 
 * 封装 gRPC 通用功能：
 * - Channel 管理
 * - 连接状态监控
 * - 超时设置
 * - 重试策略
 */

#pragma once

#include <string>
#include <memory>
#include <grpcpp/grpcpp.h>

namespace swift {
namespace zone {

/**
 * @class RpcClientBase
 * @brief gRPC 客户端基类
 */
class RpcClientBase {
public:
    virtual ~RpcClientBase() = default;

    /// 连接到服务
    bool Connect(const std::string& address);

    /// 断开连接
    void Disconnect();

    /// 检查连接状态
    bool IsConnected() const;

    /// 获取服务地址
    const std::string& GetAddress() const { return address_; }

protected:
    /// 获取 Channel（供子类创建 Stub）
    std::shared_ptr<grpc::Channel> GetChannel() { return channel_; }

    /// 创建带超时的 Context
    std::unique_ptr<grpc::ClientContext> CreateContext(int timeout_ms = 5000);

private:
    std::string address_;
    std::shared_ptr<grpc::Channel> channel_;
};

}  // namespace zone
}  // namespace swift
