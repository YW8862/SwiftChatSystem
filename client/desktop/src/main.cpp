/**
 * SwiftChat Desktop Client
 *
 * Qt5 桌面客户端入口
 * 协议层串联（设计文档 11.3 / 阶段 A.5）：
 * WebSocketClient <-> ProtocolHandler，二进制 Protobuf 收发
 */

#include <QApplication>
#include <QDebug>
#include <QStyleFactory>
#include <cstdlib>
#include <iostream>
#include "ui/loginwindow.h"
#include "network/websocket_client.h"
#include "network/protocol_handler.h"
#include "utils/settings.h"
#include "utils/client_logger.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    app.setApplicationName("SwiftChat");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Swift");
    app.setStyleSheet(
        "QWidget { background: #f3f6fb; color: #1f2430; font-family: 'Noto Sans CJK SC', 'Microsoft YaHei', sans-serif; }"
        "QToolTip { background: #1f2430; color: #ffffff; border: none; padding: 6px; border-radius: 6px; }"
        "QScrollBar:vertical { background: transparent; width: 8px; margin: 4px 0 4px 0; }"
        "QScrollBar::handle:vertical { background: #c5cede; border-radius: 4px; min-height: 24px; }"
        "QScrollBar::handle:vertical:hover { background: #a7b4c9; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar:horizontal { background: transparent; height: 8px; margin: 0 4px 0 4px; }"
        "QScrollBar::handle:horizontal { background: #c5cede; border-radius: 4px; min-width: 24px; }"
        "QScrollBar::handle:horizontal:hover { background: #a7b4c9; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
    );

    if (!client::logger::Init()) {
        std::cerr << "Failed to initialize client logger." << std::endl;
    }

    // 若无可视化显示，提前提示
    const char *display = std::getenv("DISPLAY");
    if (!display || !*display) {
        qWarning("SwiftChat: DISPLAY 未设置，Qt 需要图形环境才能显示窗口。");
        qWarning("  本机桌面：通常自动有 DISPLAY。");
        qWarning("  SSH 远程：请用 ssh -X 或 ssh -Y 启用 X11 转发，或使用 VNC/xvfb。");
        qWarning("  xvfb 示例: Xvfb :99 -screen 0 1024x768x24 & DISPLAY=:99 ./SwiftChat");
    }

    // 创建网络层组件
    WebSocketClient *wsClient = new WebSocketClient(&app);
    ProtocolHandler *protocol = new ProtocolHandler(&app);

    // 串联：ProtocolHandler 发出数据 -> WebSocketClient 发送
    QObject::connect(protocol, &ProtocolHandler::dataToSend,
                     wsClient, &WebSocketClient::sendMessage);

    // 串联：WebSocketClient 收到数据 -> ProtocolHandler 处理
    QObject::connect(wsClient, &WebSocketClient::messageReceived,
                     protocol, &ProtocolHandler::handleMessage);

    // 断线时清理未完成请求
    QObject::connect(wsClient, &WebSocketClient::disconnected,
                     protocol, &ProtocolHandler::clearPendingRequests);

    LoginWindow loginWindow(wsClient, protocol);
    loginWindow.show();
    loginWindow.raise();
    loginWindow.activateWindow();

    const int exitCode = app.exec();
    client::logger::Shutdown();
    return exitCode;
}
