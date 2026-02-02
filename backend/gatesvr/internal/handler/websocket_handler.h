#pragma once

#include <memory>
#include <string>

namespace swift::gate {

class GateService;

/**
 * WebSocket 连接处理器
 */
class WebSocketHandler {
public:
    explicit WebSocketHandler(std::shared_ptr<GateService> service);
    ~WebSocketHandler();
    
    // 新连接建立
    void OnConnect(const std::string& conn_id);
    
    // 收到客户端消息
    void OnMessage(const std::string& conn_id, const std::string& data);
    
    // 连接断开
    void OnDisconnect(const std::string& conn_id);
    
    // 发送消息给客户端
    bool SendToClient(const std::string& conn_id, const std::string& data);
    
private:
    std::shared_ptr<GateService> service_;
};

/**
 * GateInternalService gRPC 处理器
 * 
 * 供 ZoneSvr 调用，推送消息给客户端
 */
class GateInternalHandler {
public:
    explicit GateInternalHandler(std::shared_ptr<GateService> service);
    ~GateInternalHandler();
    
    // ZoneSvr 调用：推送消息给用户
    bool PushMessage(const std::string& user_id, const std::string& cmd,
                     const std::string& payload);
    
    // ZoneSvr 调用：断开用户连接
    bool DisconnectUser(const std::string& user_id, const std::string& reason);
    
private:
    std::shared_ptr<GateService> service_;
};

}  // namespace swift::gate
