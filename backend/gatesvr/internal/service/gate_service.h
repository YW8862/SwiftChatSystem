#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <shared_mutex>

namespace swift::gate {

/**
 * 连接信息
 */
struct Connection {
    std::string conn_id;
    std::string user_id;       // 登录后才有
    std::string device_type;
    int64_t connected_at;
    int64_t last_heartbeat;
    bool authenticated = false;
};

/**
 * 网关服务
 * 
 * 核心职责：
 * 1. 管理 WebSocket 连接
 * 2. 协议解析（Protobuf）
 * 3. 转发请求到 ZoneSvr
 * 4. 推送消息给客户端
 */
class GateService {
public:
    GateService();
    ~GateService();
    
    // 连接管理
    void AddConnection(const std::string& conn_id);
    void RemoveConnection(const std::string& conn_id);
    Connection* GetConnection(const std::string& conn_id);
    
    // 用户登录后绑定
    bool BindUser(const std::string& conn_id, const std::string& user_id);
    void UnbindUser(const std::string& user_id);
    
    // 根据 user_id 查找连接
    std::string GetConnIdByUser(const std::string& user_id);
    
    // 处理客户端消息
    void HandleClientMessage(const std::string& conn_id, const std::string& cmd,
                             const std::string& payload, const std::string& request_id);
    
    // 推送消息给用户
    bool PushToUser(const std::string& user_id, const std::string& cmd,
                    const std::string& payload);
    
    // 心跳检测
    void CheckHeartbeat();
    
    // 获取当前连接数
    int GetConnectionCount() const;
    
private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Connection> connections_;      // conn_id -> Connection
    std::unordered_map<std::string, std::string> user_to_conn_;    // user_id -> conn_id
    
    std::string gate_id_;
    // ZoneSvr gRPC 客户端
};

}  // namespace swift::gate
