/**
 * AuthSvr - 认证服务
 * 提供注册、校验凭证、获取/更新资料等 gRPC 接口。
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
#include "handler/auth_handler.h"
#include "service/auth_service.h"
#include "store/user_store.h"

namespace {
std::atomic<bool> g_running{true};
std::unique_ptr<grpc::Server> g_server;
} // namespace

void SignalHandler(int signal) {
  LogInfo(TAG("service", "authsvr").Add("signal", std::to_string(signal)),
          "Received signal, shutting down...");
  g_running = false;
  if (g_server) {
    g_server->Shutdown();
  }
}

int main(int argc, char *argv[]) {
  std::string config_file;
  if (argc > 1) {
    config_file = argv[1];
  } else {
    const char *env_config = std::getenv("AUTHSVR_CONFIG");
    config_file = env_config ? env_config : "authsvr.conf";
  }

  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);

  if (!swift::log::InitFromEnv("authsvr")) {
    std::cerr << "Failed to initialize logger!" << std::endl;
    return 1;
  }

  LogInfo("========================================");
  LogInfo("AuthSvr starting...");
  LogInfo("========================================");
  LogInfo(TAG("service", "authsvr"), "Using config file: " << config_file);

  swift::auth::AuthConfig config = swift::auth::LoadConfig(config_file);
  LogInfo(TAG("service", "authsvr"),
          "Config: host=" << config.host << " port=" << config.port
                          << " store=" << config.store_type
                          << " path=" << config.rocksdb_path);

  // 存储
  std::shared_ptr<swift::auth::UserStore> store;
  if (config.store_type == "rocksdb") {
    try {
      store =
          std::make_shared<swift::auth::RocksDBUserStore>(config.rocksdb_path);
      LogInfo(TAG("service", "authsvr"), "RocksDB opened: " << config.rocksdb_path);
    } catch (const std::exception &e) {
      LogError(TAG("service", "authsvr"),
               "Failed to open RocksDB: " << e.what());
      swift::log::Shutdown();
      return 1;
    }
  } else {
    LogError(TAG("service", "authsvr").Add("store_type", config.store_type),
             "Unsupported store_type");
    swift::log::Shutdown();
    return 1;
  }

  // 业务层与 Handler
  auto service_core = std::make_shared<swift::auth::AuthServiceCore>(store);
  swift::auth::AuthHandler handler(service_core, config.jwt_secret);

  // gRPC 服务
  std::string addr = config.host + ":" + std::to_string(config.port);
  grpc::ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&handler);

  g_server = builder.BuildAndStart();
  if (!g_server) {
    LogError(TAG("service", "authsvr"),
             "Failed to start gRPC server on " << addr);
    swift::log::Shutdown();
    return 1;
  }

  LogInfo(TAG("service", "authsvr"),
          "AuthSvr listening on " << addr << " (press Ctrl+C to stop)");

  g_server->Wait();

  g_server.reset();
  LogInfo(TAG("service", "authsvr"), "AuthSvr shut down.");
  swift::log::Shutdown();
  return 0;
}
