#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <shared_mutex>

namespace swift::gate {

struct GateConfig;
class ZoneRpcClient;

/** 发送回调：Session 注册，供 PushToUser/SendToConn 调用 */
using SendCallback = std::function<bool(const std::string&)>;
/** 关闭连接回调：Session 注册，供 CheckHeartbeat 超时踢线时调用 */
using CloseCallback = std::function<void()>;

/**
 * 连接信息：conn_id → (user_id, token, device_id, device_type)
 */
struct Connection {
    std::string conn_id;
    std::string user_id;       // 登录后才有
    std::string token;         // 登录时校验通过的 JWT，转发业务请求时带给 Zone 注入业务服务
    std::string device_id;     // 设备 ID
    std::string device_type;   // windows, android, ios, web
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

    /** 初始化：zone_svr_addr、zonesvr_internal_secret、gate_id，用于 OnDisconnect 通知 ZoneSvr UserOffline */
    void Init(const GateConfig& config);
    
    // 连接管理
    void AddConnection(const std::string& conn_id);
    void RemoveConnection(const std::string& conn_id);
    Connection* GetConnection(const std::string& conn_id);
    
    // 用户登录后绑定（含 token、device_id、device_type）
    bool BindUser(const std::string& conn_id, const std::string& user_id,
                  const std::string& token = "",
                  const std::string& device_id = "", const std::string& device_type = "");
    void UnbindUser(const std::string& user_id);
    
    // 根据 user_id 查找连接
    std::string GetConnIdByUser(const std::string& user_id);
    /** 根据 user_id 获取 Connection*（用于获取完整连接信息） */
    Connection* GetConnectionByUserId(const std::string& user_id);
    
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

    /** 注册 conn_id 的发送回调（WsSession 在连接建立时调用） */
    void SetSendCallback(const std::string& conn_id, SendCallback fn);
    /** 移除 conn_id 的发送回调（WsSession 在断开时调用） */
    void RemoveSendCallback(const std::string& conn_id);
    /** 注册 conn_id 的关闭回调（WsSession 注册，CheckHeartbeat 超时踢线时调用） */
    void SetCloseCallback(const std::string& conn_id, CloseCallback fn);
    /** 移除 conn_id 的关闭回调（WsSession 在 Close 时调用） */
    void RemoveCloseCallback(const std::string& conn_id);
    /** 主动关闭连接（从任意线程调用，会触发 Session Close → RemoveConnection → UserOffline） */
    void CloseConnection(const std::string& conn_id);
    /** 向指定连接发送数据（由 PushToUser、WebSocketHandler::SendToClient 使用） */
    bool SendToConn(const std::string& conn_id, const std::string& data);

    /** 向 ZoneSvr 注册本 Gate（启动时调用，传入本机 gRPC 地址） */
    bool RegisterGate(const std::string& grpc_address);

    /** 向 ZoneSvr 上报 Gate 心跳（建议每 30s 调用，与 heartbeat_interval_seconds 对齐） */
    bool GateHeartbeat();
    
private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Connection> connections_;      // conn_id -> Connection
    std::unordered_map<std::string, std::string> user_to_conn_;    // user_id -> conn_id
    std::unordered_map<std::string, SendCallback> send_callbacks_; // conn_id -> SendCallback
    
    std::string gate_id_;
    std::string zone_svr_addr_;
    std::string zonesvr_internal_secret_;
    int heartbeat_timeout_seconds_ = 90;
    std::unique_ptr<ZoneRpcClient> zone_client_;
    std::unordered_map<std::string, CloseCallback> close_callbacks_;

    void NotifyUserOffline(const std::string& user_id);
    /** 向连接发送 ServerMessage（cmd/request_id/code/message/payload） */
    bool SendResponse(const std::string& conn_id, const std::string& cmd,
                     const std::string& request_id, int code, const std::string& message,
                     const std::string& payload = "");
    void UpdateHeartbeat(const std::string& conn_id);
    void HandleLogin(const std::string& conn_id, const std::string& payload,
                     const std::string& request_id);
    void HandleHeartbeat(const std::string& conn_id, const std::string& request_id);
    void ForwardToZone(const std::string& conn_id, const std::string& cmd,
                      const std::string& payload, const std::string& request_id);
};

}  // namespace swift::gate
