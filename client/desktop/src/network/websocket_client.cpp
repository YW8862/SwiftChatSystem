#include "websocket_client.h"

WebSocketClient::WebSocketClient(QObject *parent) 
    : QObject(parent)
    , m_socket(std::make_unique<QWebSocket>()) {
    
    QObject::connect(m_socket.get(), &QWebSocket::connected, 
                     this, &WebSocketClient::onConnected);
    QObject::connect(m_socket.get(), &QWebSocket::disconnected, 
                     this, &WebSocketClient::onDisconnected);
    QObject::connect(m_socket.get(), &QWebSocket::binaryMessageReceived, 
                     this, &WebSocketClient::onBinaryMessageReceived);
    QObject::connect(m_socket.get(), QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                     this, &WebSocketClient::onError);
}

WebSocketClient::~WebSocketClient() = default;

void WebSocketClient::connect(const QString& url) {
    m_socket->open(QUrl(url));
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

void WebSocketClient::onError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error);
    emit errorOccurred(m_socket->errorString());
}
