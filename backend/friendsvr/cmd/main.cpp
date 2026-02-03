/**
 * FriendSvr - 好友服务
 *
 * 职责：
 * - 好友关系管理
 * - 好友请求处理
 * - 好友分组管理
 * - 黑名单管理
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
#include "handler/friend_handler.h"
#include "service/friend_service.h"
#include "store/friend_store.h"

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
    std::string config_file;
    if (argc > 1) {
        config_file = argv[1];
    } else {
        const char* env_config = std::getenv("FRIENDSVR_CONFIG");
        config_file = env_config ? env_config : "friendsvr.conf";
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    if (!swift::log::InitFromEnv("friendsvr")) {
        std::cerr << "Failed to initialize logger!" << std::endl;
        return 1;
    }

    LogInfo("========================================");
    LogInfo("FriendSvr starting...");
    LogInfo("========================================");

    swift::friend_::FriendConfig config = swift::friend_::LoadConfig(config_file);
    LogInfo("Config: host=" << config.host << " port=" << config.port
            << " store=" << config.store_type << " path=" << config.rocksdb_path);

    std::shared_ptr<swift::friend_::FriendStore> store;
    if (config.store_type == "rocksdb") {
        try {
            store = std::make_shared<swift::friend_::RocksDBFriendStore>(config.rocksdb_path);
            LogInfo("RocksDB opened: " << config.rocksdb_path);
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

    auto service = std::make_shared<swift::friend_::FriendService>(store);
    swift::friend_::FriendHandler handler(service);

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

    LogInfo("FriendSvr listening on " << addr << " (press Ctrl+C to stop)");

    g_server->Wait();

    g_server.reset();
    LogInfo("FriendSvr shut down.");
    swift::log::Shutdown();
    return 0;
}
