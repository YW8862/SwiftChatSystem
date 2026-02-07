/**
 * ZoneSvr - 路由服务
 *
 * 职责：用户在线状态、消息路由与广播、Gate 节点管理、会话状态。
 * 接入认证：若配置 internal_secret，则通过 AuthMetadataProcessor 校验 x-internal-secret（见 system.md 2.7）。
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include "config/config.h"
#include "interceptor/internal_secret_processor.h"

int main(int argc, char* argv[]) {
    std::string config_file = (argc > 1) ? argv[1] : "";
    if (config_file.empty()) {
        const char* env = std::getenv("ZONESVR_CONFIG");
        config_file = env ? env : "zonesvr.conf";
    }

    swift::zone::ZoneConfig config = swift::zone::LoadConfig(config_file);

    std::string addr = config.host + ":" + std::to_string(config.port);
    auto creds = grpc::InsecureServerCredentials();
    if (!config.internal_secret.empty()) {
        creds->SetAuthMetadataProcessor(
            std::make_shared<swift::zone::InternalSecretProcessor>(config.internal_secret));
    }

    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, creds);
    // RegisterService(ZoneHandler) 将在实现 ZoneHandler 与 ZoneService 后添加
    auto server = builder.BuildAndStart();
    if (!server) {
        std::cerr << "ZoneSvr failed to start on " << addr << std::endl;
        return 1;
    }
    std::cout << "ZoneSvr listening on " << addr
              << (config.internal_secret.empty() ? " (no internal auth)" : " (internal secret required)")
              << std::endl;
    server->Wait();
    return 0;
}
