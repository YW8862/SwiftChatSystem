#pragma once

#include <memory>
#include <string>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace swift::gate {

class GateService;
class WebSocketHandler;

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

/**
 * WebSocket 监听器：Boost.Beast 实现，接受连接并创建 Session。
 * 与 GateService、WebSocketHandler 配合完成连接生命周期管理。
 */
class WsListener : public std::enable_shared_from_this<WsListener> {
public:
    WsListener(net::io_context& ioc,
               const std::string& host, int port,
               std::shared_ptr<GateService> service,
               std::shared_ptr<WebSocketHandler> ws_handler);
    ~WsListener();

    void Run();
    void Stop();

private:
    void DoAccept();
    void OnAccept(beast::error_code ec, tcp::socket socket);

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::shared_ptr<GateService> service_;
    std::shared_ptr<WebSocketHandler> ws_handler_;
    bool stopped_ = false;
};

}  // namespace swift::gate
