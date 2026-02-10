/**
 * ChatSvr - 消息与群组服务
 *
 * 职责：
 * - 消息存储与查询、撤回、离线队列、会话同步等（ChatServiceCore）
 * - 群组：创建群、邀请成员、解散、成员管理等（GroupService）
 */

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <filesystem>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <swift/log_helper.h>

#include "config/config.h"
#include "handler/chat_handler.h"
#include "handler/group_handler.h"
#include "service/chat_service.h"
#include "service/group_service.h"
#include "store/group_store.h"
#include "store/message_store.h"

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
    LogError(TAG("service", "chatsvr"), "Failed to initialize logger!");
    return 1;
  }

  LogInfo("========================================");
  LogInfo("ChatSvr starting...");
  LogInfo("========================================");

  LogInfo("Using config file: " << (config_file.empty() ? "<empty>" : config_file));

  swift::chat::ChatConfig config = swift::chat::LoadConfig(config_file);
  std::string group_db_path = config.rocksdb_path + "/group";
  std::string message_db_path = config.rocksdb_path + "/message";
  std::string conv_db_path = config.rocksdb_path + "/conv";
  std::string conv_meta_db_path = config.rocksdb_path + "/conv_meta";
  LogInfo("Config: host=" << config.host << " port=" << config.port
          << " rocksdb=" << config.rocksdb_path);

  // 额外输出 RocksDB 相关路径与目录信息，便于在容器环境（如 k8s）排查挂载问题
  namespace fs = std::filesystem;
  auto logPathInfo = [](const std::string& label, const std::string& path) {
    try {
      bool exists = fs::exists(path);
      bool is_dir = exists && fs::is_directory(path);
      LogInfo(label << ": path=" << path
                    << " exists=" << (exists ? "true" : "false")
                    << " is_dir=" << (is_dir ? "true" : "false"));
    } catch (const std::exception& e) {
      LogError("Failed to stat path (" << label << "): " << path << " error=" << e.what());
    }
  };

  logPathInfo("RocksDB base dir", config.rocksdb_path);
  logPathInfo("RocksDB group path", group_db_path);
  logPathInfo("RocksDB message path", message_db_path);
  logPathInfo("RocksDB conv path", conv_db_path);
  logPathInfo("RocksDB conv_meta path", conv_meta_db_path);

  // 群组存储与服务
  std::shared_ptr<swift::group_::GroupStore> group_store;
  try {
    group_store = std::make_shared<swift::group_::RocksDBGroupStore>(group_db_path);
    LogInfo("RocksDB opened (group): " << group_db_path);
  } catch (const std::exception& e) {
    LogError("Failed to open RocksDB (group): " << e.what());
    LogError("Hint: check that the underlying volume is mounted and writable. "
             "rocksdb_path=" << config.rocksdb_path
             << " group_db_path=" << group_db_path);
    swift::log::Shutdown();
    return 1;
  }

  // 消息与会话存储（ChatServiceCore 依赖）
  std::shared_ptr<swift::chat::MessageStore> msg_store;
  std::shared_ptr<swift::chat::ConversationStore> conv_store;
  std::shared_ptr<swift::chat::ConversationRegistry> conv_registry;
  try {
    msg_store = std::make_shared<swift::chat::RocksDBMessageStore>(message_db_path);
    conv_store = std::make_shared<swift::chat::RocksDBConversationStore>(conv_db_path);
    conv_registry = std::make_shared<swift::chat::RocksDBConversationRegistry>(conv_meta_db_path);
    LogInfo("RocksDB opened: message=" << message_db_path
            << " conv=" << conv_db_path << " conv_meta=" << conv_meta_db_path);
  } catch (const std::exception& e) {
    LogError("Failed to open RocksDB (message/conv): " << e.what());
    swift::log::Shutdown();
    return 1;
  }

  auto group_service = std::make_shared<swift::group_::GroupService>(group_store);
  auto chat_service = std::make_shared<swift::chat::ChatServiceCore>(
      msg_store, conv_store, conv_registry, group_store);

  swift::group_::GroupHandler group_handler(group_service, config.jwt_secret);
  swift::chat::ChatHandler chat_handler(chat_service, config.jwt_secret);

  std::string addr = config.host + ":" + std::to_string(config.port);
  grpc::ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&group_handler);
  builder.RegisterService(&chat_handler);

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
