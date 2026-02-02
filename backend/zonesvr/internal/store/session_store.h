#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace swift::zone {

/**
 * 用户会话信息
 */
struct UserSession {
    std::string user_id;
    std::string gate_id;         // 连接的网关 ID
    std::string gate_addr;       // 网关 gRPC 地址
    std::string device_type;
    std::string device_id;
    int64_t online_at = 0;
    int64_t last_active_at = 0;
};

/**
 * 网关节点信息
 */
struct GateNode {
    std::string gate_id;
    std::string address;         // gRPC 地址
    int current_connections = 0;
    int64_t registered_at = 0;
    int64_t last_heartbeat = 0;
};

/**
 * 会话存储接口
 * 
 * 支持两种实现：
 * 1. MemorySessionStore - 单机内存存储（开发/单副本）
 * 2. RedisSessionStore  - Redis 存储（生产/多副本）
 * 
 * Redis Key 设计：
 *   session:{user_id}     -> UserSession (Hash)
 *   gate:{gate_id}        -> GateNode (Hash)
 *   gate:list             -> [gate_id...] (Set)
 */
class SessionStore {
public:
    virtual ~SessionStore() = default;
    
    // === 用户会话 ===
    virtual bool SetOnline(const UserSession& session) = 0;
    virtual bool SetOffline(const std::string& user_id) = 0;
    virtual std::optional<UserSession> GetSession(const std::string& user_id) = 0;
    virtual std::vector<UserSession> GetSessions(const std::vector<std::string>& user_ids) = 0;
    virtual bool IsOnline(const std::string& user_id) = 0;
    virtual bool UpdateLastActive(const std::string& user_id, int64_t timestamp) = 0;
    
    // === 网关节点 ===
    virtual bool RegisterGate(const GateNode& node) = 0;
    virtual bool UnregisterGate(const std::string& gate_id) = 0;
    virtual bool UpdateGateHeartbeat(const std::string& gate_id, int connections) = 0;
    virtual std::optional<GateNode> GetGate(const std::string& gate_id) = 0;
    virtual std::vector<GateNode> GetAllGates() = 0;
};

/**
 * 内存实现（单机/开发环境）
 */
class MemorySessionStore : public SessionStore {
public:
    MemorySessionStore();
    ~MemorySessionStore() override;
    
    bool SetOnline(const UserSession& session) override;
    bool SetOffline(const std::string& user_id) override;
    std::optional<UserSession> GetSession(const std::string& user_id) override;
    std::vector<UserSession> GetSessions(const std::vector<std::string>& user_ids) override;
    bool IsOnline(const std::string& user_id) override;
    bool UpdateLastActive(const std::string& user_id, int64_t timestamp) override;
    
    bool RegisterGate(const GateNode& node) override;
    bool UnregisterGate(const std::string& gate_id) override;
    bool UpdateGateHeartbeat(const std::string& gate_id, int connections) override;
    std::optional<GateNode> GetGate(const std::string& gate_id) override;
    std::vector<GateNode> GetAllGates() override;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * Redis 实现（生产/多副本环境）
 * 
 * 优点：
 * - 多个 ZoneSvr Pod 共享状态
 * - 支持过期自动清理
 * - 原子操作保证一致性
 */
class RedisSessionStore : public SessionStore {
public:
    explicit RedisSessionStore(const std::string& redis_url);
    ~RedisSessionStore() override;
    
    // ... 同上接口 ...
    bool SetOnline(const UserSession& session) override;
    bool SetOffline(const std::string& user_id) override;
    std::optional<UserSession> GetSession(const std::string& user_id) override;
    std::vector<UserSession> GetSessions(const std::vector<std::string>& user_ids) override;
    bool IsOnline(const std::string& user_id) override;
    bool UpdateLastActive(const std::string& user_id, int64_t timestamp) override;
    
    bool RegisterGate(const GateNode& node) override;
    bool UnregisterGate(const std::string& gate_id) override;
    bool UpdateGateHeartbeat(const std::string& gate_id, int connections) override;
    std::optional<GateNode> GetGate(const std::string& gate_id) override;
    std::vector<GateNode> GetAllGates() override;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace swift::zone
