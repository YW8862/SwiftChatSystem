#include "session_store.h"
#include <unordered_map>
#include <shared_mutex>

namespace swift::zone {

// ============== 内存实现 ==============

struct MemorySessionStore::Impl {
    std::unordered_map<std::string, UserSession> sessions;  // user_id -> session
    std::unordered_map<std::string, GateNode> gates;        // gate_id -> node
    mutable std::shared_mutex mutex;
};

MemorySessionStore::MemorySessionStore() : impl_(std::make_unique<Impl>()) {}
MemorySessionStore::~MemorySessionStore() = default;

bool MemorySessionStore::SetOnline(const UserSession& session) {
    std::unique_lock lock(impl_->mutex);
    impl_->sessions[session.user_id] = session;
    return true;
}

bool MemorySessionStore::SetOffline(const std::string& user_id) {
    std::unique_lock lock(impl_->mutex);
    impl_->sessions.erase(user_id);
    return true;
}

std::optional<UserSession> MemorySessionStore::GetSession(const std::string& user_id) {
    std::shared_lock lock(impl_->mutex);
    auto it = impl_->sessions.find(user_id);
    if (it != impl_->sessions.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<UserSession> MemorySessionStore::GetSessions(const std::vector<std::string>& user_ids) {
    std::shared_lock lock(impl_->mutex);
    std::vector<UserSession> result;
    for (const auto& uid : user_ids) {
        auto it = impl_->sessions.find(uid);
        if (it != impl_->sessions.end()) {
            result.push_back(it->second);
        }
    }
    return result;
}

bool MemorySessionStore::IsOnline(const std::string& user_id) {
    std::shared_lock lock(impl_->mutex);
    return impl_->sessions.count(user_id) > 0;
}

bool MemorySessionStore::UpdateLastActive(const std::string& user_id, int64_t timestamp) {
    std::unique_lock lock(impl_->mutex);
    auto it = impl_->sessions.find(user_id);
    if (it != impl_->sessions.end()) {
        it->second.last_active_at = timestamp;
        return true;
    }
    return false;
}

bool MemorySessionStore::RegisterGate(const GateNode& node) {
    std::unique_lock lock(impl_->mutex);
    impl_->gates[node.gate_id] = node;
    return true;
}

bool MemorySessionStore::UnregisterGate(const std::string& gate_id) {
    std::unique_lock lock(impl_->mutex);
    impl_->gates.erase(gate_id);
    return true;
}

bool MemorySessionStore::UpdateGateHeartbeat(const std::string& gate_id, int connections) {
    std::unique_lock lock(impl_->mutex);
    auto it = impl_->gates.find(gate_id);
    if (it != impl_->gates.end()) {
        it->second.current_connections = connections;
        // it->second.last_heartbeat = now();
        return true;
    }
    return false;
}

std::optional<GateNode> MemorySessionStore::GetGate(const std::string& gate_id) {
    std::shared_lock lock(impl_->mutex);
    auto it = impl_->gates.find(gate_id);
    if (it != impl_->gates.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<GateNode> MemorySessionStore::GetAllGates() {
    std::shared_lock lock(impl_->mutex);
    std::vector<GateNode> result;
    for (const auto& [_, node] : impl_->gates) {
        result.push_back(node);
    }
    return result;
}

// ============== Redis 实现 ==============

struct RedisSessionStore::Impl {
    std::string redis_url;
    // hiredis/redis-plus-plus 客户端
};

RedisSessionStore::RedisSessionStore(const std::string& redis_url)
    : impl_(std::make_unique<Impl>()) {
    impl_->redis_url = redis_url;
    // TODO: 连接 Redis
}

RedisSessionStore::~RedisSessionStore() = default;

// TODO: 实现 Redis 版本
bool RedisSessionStore::SetOnline(const UserSession& session) {
    // HSET session:{user_id} gate_id {gate_id} device_type {type} ...
    // EXPIRE session:{user_id} 3600
    return false;
}

bool RedisSessionStore::SetOffline(const std::string& user_id) {
    // DEL session:{user_id}
    return false;
}

std::optional<UserSession> RedisSessionStore::GetSession(const std::string& user_id) {
    // HGETALL session:{user_id}
    return std::nullopt;
}

std::vector<UserSession> RedisSessionStore::GetSessions(const std::vector<std::string>& user_ids) {
    // Pipeline HGETALL for each user
    return {};
}

bool RedisSessionStore::IsOnline(const std::string& user_id) {
    // EXISTS session:{user_id}
    return false;
}

bool RedisSessionStore::UpdateLastActive(const std::string& user_id, int64_t timestamp) {
    // HSET session:{user_id} last_active_at {timestamp}
    // EXPIRE session:{user_id} 3600
    return false;
}

bool RedisSessionStore::RegisterGate(const GateNode& node) {
    // HSET gate:{gate_id} ...
    // SADD gate:list {gate_id}
    return false;
}

bool RedisSessionStore::UnregisterGate(const std::string& gate_id) {
    // DEL gate:{gate_id}
    // SREM gate:list {gate_id}
    return false;
}

bool RedisSessionStore::UpdateGateHeartbeat(const std::string& gate_id, int connections) {
    return false;
}

std::optional<GateNode> RedisSessionStore::GetGate(const std::string& gate_id) {
    return std::nullopt;
}

std::vector<GateNode> RedisSessionStore::GetAllGates() {
    // SMEMBERS gate:list -> for each HGETALL
    return {};
}

}  // namespace swift::zone
