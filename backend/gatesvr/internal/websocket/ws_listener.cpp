/**
 * @file ws_listener.cpp
 * @brief Boost.Beast WebSocket 监听器与 Session
 *
 * Step 2: accept、async_read、async_write、close；每连接生成 conn_id，维护 Session。
 */

#include "ws_listener.h"
#include "../handler/websocket_handler.h"
#include "../service/gate_service.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <queue>
#include <sstream>

namespace swift::gate {

namespace {

static std::atomic<int64_t> g_session_seq{0};

std::string GenerateConnId() {
    int64_t seq = g_session_seq.fetch_add(1);
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::ostringstream os;
    os << "conn_" << now << "_" << seq;
    return os.str();
}

void fail(beast::error_code ec, const char* what) {
    std::cerr << "GateSvr ws: " << what << ": " << ec.message() << std::endl;
}

std::string HttpVersionToString(unsigned version) {
    std::ostringstream os;
    os << "HTTP/" << (version / 10) << "." << (version % 10);
    return os.str();
}

std::string RemoteEndpointString(const websocket::stream<beast::tcp_stream>& ws) {
    beast::error_code ec;
    auto ep = ws.next_layer().socket().remote_endpoint(ec);
    if (ec) return "unknown";
    std::ostringstream os;
    os << ep.address().to_string() << ":" << ep.port();
    return os.str();
}

class WsSession : public std::enable_shared_from_this<WsSession> {
public:
    WsSession(tcp::socket&& socket,
              std::shared_ptr<GateService> service,
              std::shared_ptr<WebSocketHandler> ws_handler)
        : ws_(std::move(socket))
        , service_(std::move(service))
        , ws_handler_(std::move(ws_handler))
        , conn_id_(GenerateConnId()) {}

    void Run() {
        net::dispatch(ws_.get_executor(),
            beast::bind_front_handler(&WsSession::OnRun, shared_from_this()));
    }

    /** async_write：供 GateService 回调，从任意线程调用，内部 post 到 strand */
    bool Send(const std::string& data) {
        auto self = shared_from_this();
        net::post(ws_.get_executor(), [self, data]() {
            self->QueueSend(data);
        });
        return true;
    }

private:
    void OnRun() {
        ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        DoReadHandshake();
    }

    void DoReadHandshake() {
        http::async_read(ws_.next_layer(), handshake_buffer_, handshake_req_,
            beast::bind_front_handler(&WsSession::OnReadHandshake, shared_from_this()));
    }

    void OnReadHandshake(beast::error_code ec, std::size_t bytes_transferred) {
        (void)bytes_transferred;
        if (ec) {
            std::cerr << "GateSvr ws: handshake-read: " << ec.message()
                      << ", remote=" << RemoteEndpointString(ws_) << std::endl;
            return;
        }

        const std::string request_line = std::string(handshake_req_.method_string())
            + " " + std::string(handshake_req_.target())
            + " " + HttpVersionToString(handshake_req_.version());

        if (!websocket::is_upgrade(handshake_req_)) {
            std::cerr << "GateSvr ws: handshake-invalid: not websocket upgrade"
                      << ", remote=" << RemoteEndpointString(ws_)
                      << ", request_line=\"" << request_line << "\""
                      << std::endl;
            beast::error_code ignored_ec;
            ws_.next_layer().socket().shutdown(tcp::socket::shutdown_both, ignored_ec);
            ws_.next_layer().socket().close(ignored_ec);
            return;
        }

        ws_.async_accept(handshake_req_,
            beast::bind_front_handler(&WsSession::OnAccept, shared_from_this()));
    }

    void OnAccept(beast::error_code ec) {
        if (ec) {
            const std::string request_line = std::string(handshake_req_.method_string())
                + " " + std::string(handshake_req_.target())
                + " " + HttpVersionToString(handshake_req_.version());
            std::cerr << "GateSvr ws: accept: " << ec.message()
                      << ", remote=" << RemoteEndpointString(ws_)
                      << ", request_line=\"" << request_line << "\""
                      << std::endl;
            return;
        }
        service_->AddConnection(conn_id_);
        service_->SetSendCallback(conn_id_, [self = shared_from_this()](const std::string& d) {
            return self->Send(d);
        });
        service_->SetCloseCallback(conn_id_, [self = shared_from_this()]() {
            net::post(self->ws_.get_executor(), [self]() { self->Close(); });
        });
        ws_handler_->OnConnect(conn_id_);
        DoRead();
    }

    void DoRead() {
        ws_.async_read(buffer_,
            beast::bind_front_handler(&WsSession::OnRead, shared_from_this()));
    }

    void OnRead(beast::error_code ec, std::size_t bytes_transferred) {
        (void)bytes_transferred;
        if (ec == websocket::error::closed) {
            Close();
            return;
        }
        if (ec) {
            fail(ec, "read");
            Close();
            return;
        }
        std::string data = beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());
        ws_handler_->OnMessage(conn_id_, data);
        DoRead();
    }

    void QueueSend(const std::string& data) {
        if (closed_) return;
        write_queue_.push(data);
        if (!writing_) DoWrite();
    }

    void DoWrite() {
        if (closed_ || write_queue_.empty()) return;
        writing_ = true;
        write_buffer_ = std::move(write_queue_.front());
        write_queue_.pop();
        ws_.async_write(net::buffer(write_buffer_),
            beast::bind_front_handler(&WsSession::OnWrite, shared_from_this()));
    }

    void OnWrite(beast::error_code ec, std::size_t bytes_transferred) {
        (void)bytes_transferred;
        writing_ = false;
        if (ec) {
            if (!closed_) {
                fail(ec, "write");
                Close();
            }
            return;
        }
        if (!write_queue_.empty())
            DoWrite();
    }

    void Close() {
        if (closed_) return;
        closed_ = true;
        service_->RemoveCloseCallback(conn_id_);
        service_->RemoveSendCallback(conn_id_);
        ws_handler_->OnDisconnect(conn_id_);
        service_->RemoveConnection(conn_id_);
        beast::error_code ec;
        ws_.close(websocket::close_code::normal, ec);
    }

    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer handshake_buffer_;
    http::request<http::string_body> handshake_req_;
    beast::flat_buffer buffer_;
    std::shared_ptr<GateService> service_;
    std::shared_ptr<WebSocketHandler> ws_handler_;
    std::string conn_id_;
    std::queue<std::string> write_queue_;
    std::string write_buffer_;
    bool writing_ = false;
    bool closed_ = false;
};

}  // namespace

// ---------------------------------------------------------------------------
// WsListener
// ---------------------------------------------------------------------------

WsListener::WsListener(net::io_context& ioc,
                       const std::string& host, int port,
                       std::shared_ptr<GateService> service,
                       std::shared_ptr<WebSocketHandler> ws_handler)
    : ioc_(ioc)
    , acceptor_(ioc)
    , service_(std::move(service))
    , ws_handler_(std::move(ws_handler)) {
    beast::error_code ec;
    std::string bind_host = host.empty() ? "0.0.0.0" : host;
    auto addr = net::ip::make_address(bind_host, ec);
    if (ec) {
        throw std::runtime_error("invalid host '" + host + "': " + ec.message());
    }
    tcp::endpoint endpoint(addr, static_cast<unsigned short>(port));
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        throw std::runtime_error("acceptor open: " + ec.message());
    }
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        throw std::runtime_error("acceptor set_option: " + ec.message());
    }
    acceptor_.bind(endpoint, ec);
    if (ec) {
        throw std::runtime_error("acceptor bind: " + ec.message());
    }
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        throw std::runtime_error("acceptor listen: " + ec.message());
    }
}

WsListener::~WsListener() = default;

void WsListener::Run() {
    DoAccept();
}

void WsListener::Stop() {
    stopped_ = true;
    beast::error_code ec;
    acceptor_.close(ec);
}

void WsListener::DoAccept() {
    if (stopped_) return;
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(&WsListener::OnAccept, shared_from_this()));
}

void WsListener::OnAccept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        if (!stopped_)
            fail(ec, "accept");
        return;
    }
    std::make_shared<WsSession>(std::move(socket), service_, ws_handler_)->Run();
    DoAccept();
}

}  // namespace swift::gate
