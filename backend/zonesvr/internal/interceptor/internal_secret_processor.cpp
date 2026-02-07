#include "internal_secret_processor.h"
#include <grpcpp/support/string_ref.h>
#include <grpcpp/support/status.h>

namespace swift::zone {

namespace {

const char kMetadataKey[] = "x-internal-secret";

}  // namespace

InternalSecretProcessor::InternalSecretProcessor(std::string expected_secret)
    : expected_(std::move(expected_secret)) {}

/**
 * @brief 处理 gRPC 请求的 metadata，校验 x-internal-secret 是否与预期一致
 * @param auth_metadata 请求的 metadata
 * @param context 请求的上下文
 * @param consumed_auth_metadata 处理后的 metadata
 * @param response_metadata 响应的 metadata
 * @return 处理结果
 */
grpc::Status InternalSecretProcessor::Process(
    const InputMetadata& auth_metadata,
    grpc::AuthContext* /*context*/,
    OutputMetadata* /*consumed_auth_metadata*/,
    OutputMetadata* /*response_metadata*/) {
    if (expected_.empty()) {
        return grpc::Status::OK;
    }
    grpc::string_ref key_ref(kMetadataKey);
    auto it = auth_metadata.find(key_ref);
    if (it == auth_metadata.end()) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                            "missing or invalid x-internal-secret");
    }
    std::string value(it->second.data(), it->second.size());
    if (value != expected_) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                            "missing or invalid x-internal-secret");
    }
    return grpc::Status::OK;
}

}  // namespace swift::zone
