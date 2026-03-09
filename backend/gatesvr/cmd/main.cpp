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
#include <unistd.h>

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

/**
 * ZoneSvr 端口可连后，重试 GateRegister（指数退避）应对下游短暂抖动：
 * 1s, 2s, 4s, 8s ...（上限 8s），总时长不超过 total_timeout_sec。
 */
bool RegisterGateWithRetry(const std::shared_ptr<swift::gate::GateService>& gate_svc,
                           const std::string& grpc_addr,
                           int total_timeout_sec) {
    if (!gate_svc) return false;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(total_timeout_sec);
    int attempt = 0;
    int backoff_sec = 1;
    while (std::chrono::steady_clock::now() < deadline) {
        attempt++;
        if (gate_svc->RegisterGate(grpc_addr)) {
            LogInfo("GateSvr registered with ZoneSvr after " << attempt << " attempt(s)");
            return true;
        }

        auto now = std::chrono::steady_clock::now();
        auto remaining = std::chrono::duration_cast<std::chrono::seconds>(deadline - now).count();
        if (remaining <= 0) break;
        int sleep_sec = backoff_sec < remaining ? backoff_sec : static_cast<int>(remaining);
        LogWarning("GateSvr GateRegister attempt " << attempt << " failed, retry in "
                   << sleep_sec << "s");
        std::this_thread::sleep_for(std::chrono::seconds(sleep_sec));
        if (backoff_sec < 8) backoff_sec *= 2;
    }
    return false;
}

bool IsWildcardHost(const std::string& host) {
    return host.empty() || host == "0.0.0.0" || host == "::";
}

std::string ResolveAdvertiseHost(const swift::gate::GateConfig& config) {
    if (!config.advertise_host.empty()) return config.advertise_host;

    const char* pod_ip = std::getenv("POD_IP");
    if (pod_ip && *pod_ip) return std::string(pod_ip);

    if (!IsWildcardHost(config.host)) return config.host;

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0 && hostname[0] != '\0') {
        return std::string(hostname);
    }
    return config.host;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string config_file = (argc > 1) ? argv[1] : "";
    if (config_file.empty()) {
        const char* env = std::getenv("GATESVR_CONFIG");
        config_file = env ? env : "gatesvr.conf";
    }

    if (!swift::log::InitFromEnv("gatesvr")) {
        std::cerr << "Failed to initialize logger!" << std::endl;
        return 1;
    }

    swift::gate::GateConfig config = swift::gate::LoadConfig(config_file);

    // GateService 与 Handler
    auto gate_svc = std::make_shared<swift::gate::GateService>();
    gate_svc->Init(config);
    auto ws_handler = std::make_shared<swift::gate::WebSocketHandler>(gate_svc);
    auto grpc_handler = std::make_shared<swift::gate::GateInternalGrpcHandler>(gate_svc);

    // gRPC 服务监听地址（可为 0.0.0.0），以及对 Zone 上报地址（必须可路由）
    std::string grpc_addr = config.host + ":" + std::to_string(config.grpc_port);
    const std::string advertise_host = ResolveAdvertiseHost(config);
    const std::string register_addr = advertise_host + ":" + std::to_string(config.grpc_port);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(grpc_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(grpc_handler.get());
    auto grpc_server = builder.BuildAndStart();
    if (!grpc_server) {
        std::cerr << "GateSvr gRPC failed to start on " << grpc_addr << std::endl;
        return 1;
    }
    std::cout << "GateSvr gRPC listening on " << grpc_addr << std::endl;
    LogInfo("GateSvr register address resolved to " << register_addr
            << " (listen=" << grpc_addr << ")");
    if (IsWildcardHost(advertise_host)) {
        LogWarning("GateSvr register address host is wildcard (" << advertise_host
                   << "); ZoneSvr may fail to callback this gate");
    }

    // 先等待 ZoneSvr 就绪（gRPC 端口可连），再注册；超时则报错
    const int zone_ready_timeout_sec = 30;
    const int zone_ready_poll_interval_sec = 2;
    const int gate_register_retry_timeout_sec = 60;
    if (!config.zone_svr_addr.empty()) {
        LogInfo("GateSvr waiting for ZoneSvr at " << config.zone_svr_addr
                  << " (timeout " << zone_ready_timeout_sec << "s) ...");
        if (WaitForZoneSvrReady(config.zone_svr_addr, zone_ready_timeout_sec, zone_ready_poll_interval_sec)) {
            if (RegisterGateWithRetry(gate_svc, register_addr, gate_register_retry_timeout_sec)) {
                LogInfo("GateSvr registration flow completed");
            } else {
                LogError("GateSvr ZoneSvr GateRegister failed after retries (ZoneSvr ready but register RPC failed)");
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

    swift::log::Shutdown();
    return 0;
}
