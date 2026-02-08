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
#include "handler/zone_handler.h"
#include "interceptor/internal_secret_processor.h"
#include "service/zone_service.h"
#include "system/system_manager.h"

int main(int argc, char* argv[]) {
    std::string config_file = (argc > 1) ? argv[1] : "";
    if (config_file.empty()) {
        const char* env = std::getenv("ZONESVR_CONFIG");
        config_file = env ? env : "zonesvr.conf";
    }

    swift::zone::ZoneConfig config = swift::zone::LoadConfig(config_file);

    // Step 13：先 SystemManager Init，再组装 ZoneService / Handler，最后 ServerBuilder 先认证再 RegisterService
    swift::zone::SystemManager manager;
    if (!manager.Init(config)) {
        std::cerr << "ZoneSvr SystemManager Init failed" << std::endl;
        return 1;
    }

    auto zone_svc = std::make_shared<swift::zone::ZoneServiceImpl>(
        manager.GetSessionStore(), &manager);
    auto handler = std::make_shared<swift::zone::ZoneHandler>(zone_svc);

    std::string addr = config.host + ":" + std::to_string(config.port);
    auto creds = grpc::InsecureServerCredentials();
    if (!config.internal_secret.empty()) {
        creds->SetAuthMetadataProcessor(
            std::make_shared<swift::zone::InternalSecretProcessor>(config.internal_secret));
    }

    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, creds);
    // 先认证（已通过 SetAuthMetadataProcessor 注入），再注册服务
    builder.RegisterService(handler.get());
    auto server = builder.BuildAndStart();
    if (!server) {
        std::cerr << "ZoneSvr failed to start on " << addr << std::endl;
        return 1;
    }
    std::cout << "ZoneSvr listening on " << addr
              << (config.internal_secret.empty() ? " (no internal auth)" : " (internal secret required)")
              << std::endl;
    server->Wait();
    manager.Shutdown();
    return 0;
}
