#pragma once

#include <grpcpp/security/auth_metadata_processor.h>
#include <string>

namespace swift::zone {

/**
 * AuthMetadataProcessor：校验 gRPC 请求 metadata 中的 x-internal-secret，
 * 与配置的 expected_secret 一致才放行（expected_secret 为空则不校验）。
 * 用于 ZoneSvr 只接受来自 GateSvr 等持有内网密钥的调用方。
 * 见 system.md 2.7、develop.md 9.8.1。
 */
class InternalSecretProcessor : public grpc::AuthMetadataProcessor {
public:
    explicit InternalSecretProcessor(std::string expected_secret);

    bool IsBlocking() const override { return false; }

    grpc::Status Process(
        const InputMetadata& auth_metadata,
        grpc::AuthContext* context,
        OutputMetadata* consumed_auth_metadata,
        OutputMetadata* response_metadata) override;

private:
    std::string expected_;
};

}  // namespace swift::zone
