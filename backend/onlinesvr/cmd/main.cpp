/**
 * OnlineSvr - 登录会话服务
 * 提供 Login / Logout / ValidateToken，会话存储与 Token 签发。
 */

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <swift/log_helper.h>

std::atomic<bool> g_running{true};

void SignalHandler(int signal) {
    LogInfo("Received signal " << signal << ", shutting down...");
    g_running = false;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    if (!swift::log::InitFromEnv("onlinesvr")) {
        std::cerr << "Failed to initialize logger!" << std::endl;
        return 1;
    }

    LogInfo("========================================");
    LogInfo("OnlineSvr starting...");
    LogInfo("========================================");

    int port = 9095;
    LogInfo("OnlineSvr port: " << port << " (press Ctrl+C to stop)");

    int count = 0;
    while (g_running && count < 5) {
        LogDebug("Heartbeat " << count);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        count++;
    }

    LogInfo("OnlineSvr shutting down...");
    swift::log::Shutdown();
    return 0;
}
