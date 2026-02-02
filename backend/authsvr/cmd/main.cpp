/**
 * AuthSvr - 认证服务（日志测试版）
 */

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

// 只需引用这一个头文件
#include <swift/log_helper.h>

std::atomic<bool> g_running{true};

void SignalHandler(int signal) {
    LogInfo("Received signal " << signal << ", shutting down...");
    g_running = false;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    // 注册信号处理
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    // 初始化日志
    if (!swift::log::InitFromEnv("authsvr")) {
        std::cerr << "Failed to initialize logger!" << std::endl;
        return 1;
    }
    
    LogInfo("========================================");
    LogInfo("AuthSvr starting...");
    LogInfo("========================================");
    
    // 测试各级别日志
    LogTrace("This is TRACE level");
    LogDebug("This is DEBUG level");
    LogInfo("This is INFO level");
    LogWarning("This is WARNING level");
    LogError("This is ERROR level");
    
    // 测试带变量的日志
    int port = 9094;
    std::string version = "1.0.0";
    LogInfo("Server version: " << version << ", port: " << port);
    
    // 测试带 Tag 的日志
    LogInfo(TAG("service", "authsvr"), "Service initialized");
    LogInfo(TAG("user", "alice").Add("action", "login"), "User action logged");
    
    LogInfo("AuthSvr is running (press Ctrl+C to stop)");
    
    // 简单运行几秒后退出（测试用）
    int count = 0;
    while (g_running && count < 5) {
        LogDebug("Heartbeat " << count);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        count++;
    }
    
    LogInfo("AuthSvr shutting down...");
    swift::log::Shutdown();
    
    std::cout << "Done! Check ./logs/ for log files." << std::endl;
    return 0;
}
