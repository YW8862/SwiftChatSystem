#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../store/session_store.h"

namespace swift::zone {

/**
 * 路由服务业务逻辑
 * 
 * 核心职责：
 * 1. 维护用户在线状态
 * 2. 管理 Gate 节点
 * 3. 路由消息到正确的 Gate
 */
class ZoneService {
public:
    explicit ZoneService(std::shared_ptr<SessionStore> store);
    ~ZoneService();
    
    // 用户上线
    bool UserOnline(const std::string& user_id, const std::string& gate_id,
                    const std::string& device_type, const std::string& device_id);
    
    // 用户下线
    bool UserOffline(const std::string& user_id, const std::string& gate_id);
    
    // 获取用户会话（用于消息路由）
    std::optional<UserSession> GetUserSession(const std::string& user_id);
    
    // 批量获取用户状态
    std::vector<UserSession> GetUserStatuses(const std::vector<std::string>& user_ids);
    
    // 路由消息给单个用户
    struct RouteResult {
        bool delivered;       // 是否成功投递
        bool user_online;     // 用户是否在线
        std::string gate_id;  // 用户所在 Gate
    };
    RouteResult RouteToUser(const std::string& user_id, const std::string& cmd,
                            const std::string& payload);
    
    // 广播消息给多个用户
    struct BroadcastResult {
        int online_count;
        int delivered_count;
    };
    BroadcastResult Broadcast(const std::vector<std::string>& user_ids,
                              const std::string& cmd, const std::string& payload);
    
    // Gate 管理
    bool RegisterGate(const std::string& gate_id, const std::string& address);
    bool GateHeartbeat(const std::string& gate_id, int connections);
    
private:
    // 调用 Gate 的 gRPC 接口推送消息
    bool PushToGate(const std::string& gate_addr, const std::string& user_id,
                    const std::string& cmd, const std::string& payload);
    
    std::shared_ptr<SessionStore> store_;
};

}  // namespace swift::zone
