/**
 * SwiftChat Desktop Client
 *
 * Qt5 桌面客户端入口
 * 协议层串联（设计文档 11.3 / 阶段 A.5）：
 * WebSocketClient <-> ProtocolHandler，二进制 Protobuf 收发
 */

#include <QApplication>
#include "ui/loginwindow.h"
#include "network/websocket_client.h"
#include "network/protocol_handler.h"
#include "utils/settings.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("SwiftChat");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Swift");

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

    return app.exec();
}
