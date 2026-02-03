/**
 * ChatSvr - 消息与群组服务
 *
 * 职责：
 * - 消息存储与查询、撤回、离线队列等（TODO）
 * - 群组：创建群（至少 3 人）、邀请成员、解散、成员管理等
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
#include "handler/group_handler.h"
#include "service/group_service.h"
#include "store/group_store.h"

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
    const char* env_config = std::getenv("CHATSVR_CONFIG");
    config_file = env_config ? env_config : "chatsvr.conf";
  }

  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);

  if (!swift::log::InitFromEnv("chatsvr")) {
    std::cerr << "Failed to initialize logger!" << std::endl;
    return 1;
  }

  LogInfo("========================================");
  LogInfo("ChatSvr starting...");
  LogInfo("========================================");

  swift::chat::ChatConfig config = swift::chat::LoadConfig(config_file);
  std::string group_db_path = config.rocksdb_path + "/group";
  LogInfo("Config: host=" << config.host << " port=" << config.port
          << " rocksdb=" << config.rocksdb_path << " group_db=" << group_db_path);

  std::shared_ptr<swift::group_::GroupStore> group_store;
  try {
    group_store = std::make_shared<swift::group_::RocksDBGroupStore>(group_db_path);
    LogInfo("RocksDB opened: " << group_db_path);
  } catch (const std::exception& e) {
    LogError("Failed to open RocksDB: " << e.what());
    swift::log::Shutdown();
    return 1;
  }

  auto group_service = std::make_shared<swift::group_::GroupService>(group_store);
  swift::group_::GroupHandler group_handler(group_service);

  std::string addr = config.host + ":" + std::to_string(config.port);
  grpc::ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&group_handler);

  g_server = builder.BuildAndStart();
  if (!g_server) {
    LogError("Failed to start gRPC server on " << addr);
    swift::log::Shutdown();
    return 1;
  }

  LogInfo("ChatSvr listening on " << addr << " (press Ctrl+C to stop)");

  g_server->Wait();

  g_server.reset();
  LogInfo("ChatSvr shut down.");
  swift::log::Shutdown();
  return 0;
}
