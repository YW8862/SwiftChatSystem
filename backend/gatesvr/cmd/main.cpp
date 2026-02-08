/**
 * GateSvr - 接入网关服务
 *
 * 职责：
 * - WebSocket 连接管理（9090）
 * - gRPC 服务供 ZoneSvr 调用（9091）
 * - 协议解析、心跳检测、消息转发
 *
 * 配置：GATESVR_CONFIG 或 argv[1]；环境变量 GATESVR_* 覆盖。
 */

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <boost/asio.hpp>

#include "config/config.h"
#include "handler/gate_internal_grpc_handler.h"
#include "handler/websocket_handler.h"
#include "service/gate_service.h"
#include "websocket/ws_listener.h"

int main(int argc, char* argv[]) {
    std::string config_file = (argc > 1) ? argv[1] : "";
    if (config_file.empty()) {
        const char* env = std::getenv("GATESVR_CONFIG");
        config_file = env ? env : "gatesvr.conf";
    }

    swift::gate::GateConfig config = swift::gate::LoadConfig(config_file);

    // GateService 与 Handler
    auto gate_svc = std::make_shared<swift::gate::GateService>();
    gate_svc->Init(config);
    auto ws_handler = std::make_shared<swift::gate::WebSocketHandler>(gate_svc);
    auto grpc_handler = std::make_shared<swift::gate::GateInternalGrpcHandler>(gate_svc);

    // gRPC 服务：9091
    std::string grpc_addr = config.host + ":" + std::to_string(config.grpc_port);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(grpc_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(grpc_handler.get());
    auto grpc_server = builder.BuildAndStart();
    if (!grpc_server) {
        std::cerr << "GateSvr gRPC failed to start on " << grpc_addr << std::endl;
        return 1;
    }
    std::cout << "GateSvr gRPC listening on " << grpc_addr << std::endl;

    if (gate_svc->RegisterGate(grpc_addr)) {
        std::cout << "GateSvr registered with ZoneSvr" << std::endl;
    } else if (!config.zone_svr_addr.empty()) {
        std::cerr << "GateSvr ZoneSvr GateRegister failed (ZoneSvr may be down)" << std::endl;
    }

    // WebSocket 监听：9090
    boost::asio::io_context ioc{1};
    auto ws_listener = std::make_shared<swift::gate::WsListener>(
        ioc, config.host, config.websocket_port, gate_svc, ws_handler);
    ws_listener->Run();
    std::cout << "GateSvr WebSocket listening on " << config.host << ":"
              << config.websocket_port << std::endl;

    // WebSocket 在独立线程运行
    std::atomic<bool> running{true};
    std::thread ws_thread([&ioc, &running]() {
        while (running) {
            ioc.run();
            break;
        }
    });

    // 心跳：① 客户端超时检测 CheckHeartbeat；② 向 ZoneSvr 上报 Gate 存活 GateHeartbeat（建议每 30s）
    int heartbeat_interval = config.heartbeat_interval_seconds > 0
        ? config.heartbeat_interval_seconds : 30;
    std::thread heartbeat_thread([&running, gate_svc, heartbeat_interval]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(heartbeat_interval));
            if (!running) break;
            gate_svc->CheckHeartbeat();
            gate_svc->GateHeartbeat();
        }
    });

    // 主线程等待 gRPC
    grpc_server->Wait();

    running = false;
    ws_listener->Stop();
    ioc.stop();
    if (ws_thread.joinable())
        ws_thread.join();
    if (heartbeat_thread.joinable())
        heartbeat_thread.join();

    return 0;
}
