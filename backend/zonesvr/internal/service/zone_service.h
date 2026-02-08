#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "../store/session_store.h"
#include "../rpc/gate_rpc_client.h"

namespace swift::zone {

class SystemManager;

/**
 * 路由服务业务逻辑（与 proto 生成的 ZoneService 区分，故名 ZoneServiceImpl）
 *
 * 核心职责：
 * 1. 维护用户在线状态
 * 2. 管理 Gate 节点
 * 3. 路由消息到正确的 Gate
 *
 * 持有 SessionStore 与 SystemManager*，便于 PushToGate 时通过 ChatSystem 推送。
 */
class ZoneServiceImpl {
public:
    /// @param store 会话存储
    /// @param manager 可为 nullptr；非空时用于消息路由（如 PushToGate 调 ChatSystem->PushToUser）
    ZoneServiceImpl(std::shared_ptr<SessionStore> store, SystemManager* manager = nullptr);
    ~ZoneServiceImpl();
    
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

    // 踢用户下线（通知 Gate 断开连接并清理会话）
    bool KickUser(const std::string& user_id, const std::string& reason);

    /**
     * 客户端业务请求统一入口：按 cmd 分发到对应 System，各 System 通过 gRPC 调后端。
     * @return code, message, payload（业务返回体序列化），request_id 由调用方回填
     */
    struct HandleClientRequestResult {
        int32_t code = 0;
        std::string message;
        std::string payload;
        std::string request_id;  // 原样回填给调用方
    };
    HandleClientRequestResult HandleClientRequest(const std::string& conn_id,
                                                  const std::string& user_id,
                                                  const std::string& cmd,
                                                  const std::string& payload,
                                                  const std::string& request_id,
                                                  const std::string& token);

private:
    // 按域分发：各域内再按 cmd 细分，通过 gRPC 调对应 System/后端；token 供调业务服务时注入 metadata
    HandleClientRequestResult HandleAuth(const std::string& user_id, const std::string& cmd,
                                         const std::string& payload, const std::string& request_id,
                                         const std::string& token);
    HandleClientRequestResult HandleChat(const std::string& user_id, const std::string& cmd,
                                         const std::string& payload, const std::string& request_id,
                                         const std::string& token);
    HandleClientRequestResult HandleFriend(const std::string& user_id, const std::string& cmd,
                                          const std::string& payload, const std::string& request_id,
                                          const std::string& token);
    HandleClientRequestResult HandleGroup(const std::string& user_id, const std::string& cmd,
                                         const std::string& payload, const std::string& request_id,
                                         const std::string& token);
    HandleClientRequestResult HandleFile(const std::string& user_id, const std::string& cmd,
                                        const std::string& payload, const std::string& request_id,
                                        const std::string& token);

    static HandleClientRequestResult NotImplemented(const std::string& cmd,
                                                    const std::string& request_id);

    using CmdHandlerFn = HandleClientRequestResult (ZoneServiceImpl::*)(
        const std::string&, const std::string&, const std::string&, const std::string&, const std::string&);
    static const std::unordered_map<std::string, CmdHandlerFn>& GetPrefixHandlers();

    bool PushToGate(const std::string& gate_addr, const std::string& user_id,
                    const std::string& cmd, const std::string& payload);

    std::shared_ptr<GateRpcClient> GetOrCreateGateClient(const std::string& gate_addr);

    std::shared_ptr<SessionStore> store_;
    SystemManager* manager_ = nullptr;
    std::mutex gate_clients_mutex_;
    std::unordered_map<std::string, std::shared_ptr<GateRpcClient>> gate_clients_;
};

}  // namespace swift::zone
