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

#include <swift/log_helper.h>
#include "config/config.h"
#include "handler/gate_internal_grpc_handler.h"
#include "handler/websocket_handler.h"
#include "service/gate_service.h"
#include "websocket/ws_listener.h"

namespace {

/** 尝试 TCP 连接 host:port，单次超时 connect_timeout_sec 秒 */
bool TryConnectTcp(const std::string& host, const std::string& port_str, int connect_timeout_sec) {
    namespace asio = boost::asio;
    using asio::ip::tcp;
    asio::io_context ioc;
    boost::system::error_code ec;
    tcp::resolver resolver(ioc);
    auto endpoints = resolver.resolve(host, port_str, ec);
    if (ec || endpoints.empty()) {
        LogDebug("failed to resolve host: " << host << ":" << port_str << " (error: " << ec.message() << ")");
        return false;
    }
    tcp::socket socket(ioc);
    asio::steady_timer timer(ioc);
    timer.expires_after(std::chrono::seconds(connect_timeout_sec));
    bool done = false;
    bool ok = false;
    asio::async_connect(socket, endpoints, [&](const boost::system::error_code& e, const tcp::endpoint&) {
        if (done) return;
        done = true;
        timer.cancel(ec);
        ok = !e;
    });
    LogDebug("TryConnectTcp: " << host << ":" << port_str << " (timeout " << connect_timeout_sec << "s)");
    timer.async_wait([&](const boost::system::error_code& e) {
        if (done) return;
        done = true;
        boost::system::error_code ign;
        socket.close(ign);
        if (!e) timer.cancel(ign);
    });
    LogDebug("TryConnectTcp: " << host << ":" << port_str << " (timeout " << connect_timeout_sec << "s)");
    ioc.run();
    return ok;
}

/**
 * 等待 ZoneSvr 就绪（gRPC 端口可连），超时则返回 false。
 * @param zone_svr_addr 例如 "zonesvr:9092"
 * @param timeout_sec 总等待超时（秒）
 * @param poll_interval_sec 每次尝试间隔（秒）；单次连接超时取 min(2, poll_interval_sec)
 */
bool WaitForZoneSvrReady(const std::string& zone_svr_addr, int timeout_sec, int poll_interval_sec) {
    if (zone_svr_addr.empty()) return false;

    // 解析 zone_svr_addr 为 host 和 port
    size_t colon = zone_svr_addr.rfind(':');
    if (colon == std::string::npos || colon >= zone_svr_addr.size() - 1)
        return false;
    std::string host = zone_svr_addr.substr(0, colon);
    std::string port_str = zone_svr_addr.substr(colon + 1);
   
    if (host.empty() || port_str.empty()) return false;

    int connect_timeout = (poll_interval_sec > 2) ? 2 : (poll_interval_sec > 0 ? poll_interval_sec : 1);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
    LogDebug("waiting for ZoneSvr to be ready: " << zone_svr_addr << " (host: " << host << ", port: " << port_str << ")");
    while (std::chrono::steady_clock::now() < deadline) {
        if (TryConnectTcp(host, port_str, connect_timeout)){
            LogDebug("ZoneSvr is ready: " << zone_svr_addr << " (host: " << host << ", port: " << port_str << ")");
            return true;
        }
            
        if (std::chrono::steady_clock::now() >= deadline) break;
        LogInfo("failed to connect to ZoneSvr,ready to retry: " << zone_svr_addr << " (host: " << host << ", port: " << port_str << ")");
        std::this_thread::sleep_for(std::chrono::seconds(poll_interval_sec));
    }
    LogDebug("failed to connect to ZoneSvr: " << zone_svr_addr << " (host: " << host << ", port: " << port_str << ")");
    return false;
}

}  // namespace

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

    // 先等待 ZoneSvr 就绪（gRPC 端口可连），再注册；超时则报错
    const int zone_ready_timeout_sec = 30;
    const int zone_ready_poll_interval_sec = 2;
    if (!config.zone_svr_addr.empty()) {
        LogInfo("GateSvr waiting for ZoneSvr at " << config.zone_svr_addr
                  << " (timeout " << zone_ready_timeout_sec << "s) ...");
        if (WaitForZoneSvrReady(config.zone_svr_addr, zone_ready_timeout_sec, zone_ready_poll_interval_sec)) {
            if (gate_svc->RegisterGate(grpc_addr)) {
                LogInfo("GateSvr registered with ZoneSvr");
            } else {
                LogError("GateSvr ZoneSvr GateRegister failed (ZoneSvr ready but register RPC failed)");
            }
        } else {
            LogError("GateSvr ZoneSvr not ready within " << zone_ready_timeout_sec
                      << "s (timeout)");
        }
    }

    // WebSocket 监听：9090
    boost::asio::io_context ioc{1};
    auto ws_listener = std::make_shared<swift::gate::WsListener>(
        ioc, config.host, config.websocket_port, gate_svc, ws_handler);
    ws_listener->Run();
    LogInfo("GateSvr WebSocket listening on " << config.host << ":"
              << config.websocket_port);

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
