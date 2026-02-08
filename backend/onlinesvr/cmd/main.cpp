/**
 * OnlineSvr - 登录会话服务
 * 提供 Login / Logout / ValidateToken，会话存储与 Token 签发。
 */

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <swift/log_helper.h>

#include "config/config.h"
#include "handler/online_handler.h"
#include "service/online_service.h"
#include "store/session_store.h"

namespace {
std::atomic<bool> g_running{true};
std::unique_ptr<grpc::Server> g_server;
}  // namespace

void SignalHandler(int signal) {
    LogInfo("Received signal " << signal << ", shutting down...");
    g_running = false;
    if (g_server) {
        g_server->Shutdown();
    }
}

int main(int argc, char* argv[]) {
    std::string config_file = (argc > 1) ? argv[1] : "";
    if (config_file.empty()) {
        const char* env_config = std::getenv("ONLINESVR_CONFIG");
        config_file = env_config ? env_config : "onlinesvr.conf";
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    if (!swift::log::InitFromEnv("onlinesvr")) {
        std::cerr << "Failed to initialize logger!" << std::endl;
        return 1;
    }

    LogInfo("========================================");
    LogInfo("OnlineSvr starting...");
    LogInfo("========================================");

    swift::online::OnlineConfig config = swift::online::LoadConfig(config_file);
    LogInfo("Config: host=" << config.host << " port=" << config.port
            << " store=" << config.store_type << " path=" << config.rocksdb_path);

    std::shared_ptr<swift::online::SessionStore> store;
    if (config.store_type == "rocksdb") {
        try {
            store = std::make_shared<swift::online::RocksDBSessionStore>(config.rocksdb_path);
            LogInfo("RocksDB session store opened: " << config.rocksdb_path);
        } catch (const std::exception& e) {
            LogError("Failed to open RocksDB: " << e.what());
            swift::log::Shutdown();
            return 1;
        }
    } else {
        LogError("Unsupported store_type: " << config.store_type);
        swift::log::Shutdown();
        return 1;
    }

    auto service_core = std::make_shared<swift::online::OnlineServiceCore>(store, config.jwt_secret);
    swift::online::OnlineHandler handler(service_core);

    std::string addr = config.host + ":" + std::to_string(config.port);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&handler);

    g_server = builder.BuildAndStart();
    if (!g_server) {
        LogError("Failed to start gRPC server on " << addr);
        swift::log::Shutdown();
        return 1;
    }

    LogInfo("OnlineSvr listening on " << addr << " (press Ctrl+C to stop)");

    g_server->Wait();

    g_server.reset();
    LogInfo("OnlineSvr shut down.");
    swift::log::Shutdown();
    return 0;
}
