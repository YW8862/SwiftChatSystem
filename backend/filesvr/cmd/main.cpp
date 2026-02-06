/**
 * FileSvr - 文件服务
 * 提供文件上传（gRPC 流式）、下载（HTTP）、元信息管理等。
 */

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/security/server_credentials.h>
#include <swift/log_helper.h>

#include "config/config.h"
#include "handler/file_handler.h"
#include "service/file_service.h"
#include "store/file_store.h"

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
    const char* env_config = std::getenv("FILESVR_CONFIG");
    config_file = env_config ? env_config : "filesvr.conf";
  }

  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);

  if (!swift::log::InitFromEnv("filesvr")) {
    std::cerr << "Failed to initialize logger!" << std::endl;
    return 1;
  }

  LogInfo("========================================");
  LogInfo("FileSvr starting...");
  LogInfo("========================================");

  swift::file::FileConfig config = swift::file::LoadConfig(config_file);
  LogInfo("Config: host=" << config.host << " grpc_port=" << config.grpc_port
          << " http_port=" << config.http_port
          << " storage=" << config.storage_path
          << " max_file_size=" << config.max_file_size);

  std::shared_ptr<swift::file::FileStore> store;
  if (config.store_type == "rocksdb") {
    try {
      store = std::make_shared<swift::file::RocksDBFileStore>(config.rocksdb_path);
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

  auto file_service = std::make_shared<swift::file::FileServiceCore>(store, config);
  swift::file::FileHandler handler(file_service, config.jwt_secret);

  std::string addr = config.host + ":" + std::to_string(config.grpc_port);
  grpc::ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&handler);

  g_server = builder.BuildAndStart();
  if (!g_server) {
    LogError("Failed to start gRPC server on " << addr);
    swift::log::Shutdown();
    return 1;
  }

  LogInfo("FileSvr gRPC listening on " << addr << " (press Ctrl+C to stop)");
  LogInfo("HTTP download (GET /files/{file_id}) can be wired via HttpDownloadHandler when HTTP server is added.");

  g_server->Wait();

  g_server.reset();
  LogInfo("FileSvr shut down.");
  swift::log::Shutdown();
  return 0;
}
