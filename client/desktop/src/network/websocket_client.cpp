#include "websocket_client.h"
#include <QUrl>

WebSocketClient::WebSocketClient(QObject *parent) 
    : QObject(parent)
    , m_socket(std::make_unique<QWebSocket>()) {
    
    QObject::connect(m_socket.get(), &QWebSocket::connected, 
                     this, &WebSocketClient::onConnected);
    QObject::connect(m_socket.get(), &QWebSocket::disconnected, 
                     this, &WebSocketClient::onDisconnected);
    QObject::connect(m_socket.get(), &QWebSocket::binaryMessageReceived, 
                     this, &WebSocketClient::onBinaryMessageReceived);
    QObject::connect(m_socket.get(), &QWebSocket::textMessageReceived,
                     this, &WebSocketClient::onTextMessageReceived);
    QObject::connect(m_socket.get(), QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                     this, &WebSocketClient::onError);
}

WebSocketClient::~WebSocketClient() = default;

void WebSocketClient::connect(const QString& url) {
    QUrl parsed = QUrl::fromUserInput(url.trimmed());
    if (!parsed.isValid() || parsed.scheme().toLower() != "ws" || parsed.path() != "/ws") {
        emit errorOccurred("Invalid WebSocket URL. Expected format: ws://host:port/ws");
        return;
    }
    m_socket->open(parsed);
}

void WebSocketClient::disconnect() {
    m_socket->close();
}

bool WebSocketClient::isConnected() const {
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void WebSocketClient::sendMessage(const QByteArray& data) {
    m_socket->sendBinaryMessage(data);
}

void WebSocketClient::onConnected() {
    emit connected();
}

void WebSocketClient::onDisconnected() {
    emit disconnected();
}

void WebSocketClient::onBinaryMessageReceived(const QByteArray& message) {
    emit messageReceived(message);
}

void WebSocketClient::onTextMessageReceived(const QString& message) {
    Q_UNUSED(message);
    emit errorOccurred("Text frame is not supported, binary frame required.");
    m_socket->close();
}

void WebSocketClient::onError(QAbstractSocket::SocketError error) {
    QString msg = m_socket->errorString();
    if (error == QAbstractSocket::ConnectionRefusedError) {
        msg = QString("ConnectionRefused: %1").arg(msg);
    } else if (error == QAbstractSocket::HostNotFoundError) {
        msg = QString("HostNotFound: %1").arg(msg);
    } else if (error == QAbstractSocket::SocketTimeoutError) {
        msg = QString("Timeout: %1").arg(msg);
    }
    emit errorOccurred(msg);
}
