#include "session_store.h"
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#if defined(ZONESVR_USE_HIREDIS) && ZONESVR_USE_HIREDIS
#include <hiredis/hiredis.h>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <string_view>
#endif

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
        it->second.last_heartbeat = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
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

// ============== Redis 实现（system.md 5.2 Key 设计）==============

#if defined(ZONESVR_USE_HIREDIS) && ZONESVR_USE_HIREDIS

namespace {

constexpr int kSessionExpireSeconds = 3600;
constexpr int kGateExpireSeconds = 60;
constexpr char kSessionPrefix[] = "session:";
constexpr char kGatePrefix[] = "gate:";
constexpr char kGateListKey[] = "gate:list";

void ParseRedisUrl(const std::string& redis_url, std::string* host, int* port) {
    *port = 6379;
    std::string_view v(redis_url);
    if (v.substr(0, 8) == "redis://") {
        v.remove_prefix(8);
    }
    size_t colon = v.find(':');
    if (colon != std::string_view::npos) {
        *host = std::string(v.substr(0, colon));
        int p = std::atoi(v.substr(colon + 1).data());
        if (p > 0 && p < 65536) *port = p;
    } else {
        *host = std::string(v);
    }
}

}  // namespace

struct RedisSessionStore::Impl {
    std::string redis_url;
    std::string host;
    int port = 6379;
    redisContext* ctx = nullptr;
    std::mutex mutex;

    bool EnsureConnected() {
        if (ctx != nullptr) return true;
        ctx = redisConnect(host.c_str(), port);
        return (ctx != nullptr && ctx->err == 0);
    }

    redisReply* Command(const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        redisReply* reply = static_cast<redisReply*>(redisvCommand(ctx, fmt, ap));
        va_end(ap);
        return reply;
    }
};

RedisSessionStore::RedisSessionStore(const std::string& redis_url)
    : impl_(std::make_unique<Impl>()) {
    impl_->redis_url = redis_url;
    ParseRedisUrl(redis_url, &impl_->host, &impl_->port);
    impl_->EnsureConnected();
}

RedisSessionStore::~RedisSessionStore() {
    if (impl_->ctx) {
        redisFree(impl_->ctx);
        impl_->ctx = nullptr;
    }
}

bool RedisSessionStore::SetOnline(const UserSession& session) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->EnsureConnected()) return false;
    std::string key = std::string(kSessionPrefix) + session.user_id;
    redisReply* r = impl_->Command("HMSET %s user_id %s gate_id %s gate_addr %s device_type %s device_id %s online_at %lld last_active_at %lld",
                                   key.c_str(), session.user_id.c_str(), session.gate_id.c_str(),
                                   session.gate_addr.c_str(), session.device_type.c_str(),
                                   session.device_id.c_str(),
                                   static_cast<long long>(session.online_at),
                                   static_cast<long long>(session.last_active_at));
    if (!r) return false;
    bool ok = (r->type == REDIS_REPLY_STATUS || r->type == REDIS_REPLY_STRING);
    freeReplyObject(r);
    if (!ok) return false;
    r = impl_->Command("EXPIRE %s %d", key.c_str(), kSessionExpireSeconds);
    if (r) { ok = (r->type == REDIS_REPLY_INTEGER && r->integer == 1); freeReplyObject(r); }
    return ok;
}

bool RedisSessionStore::SetOffline(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->EnsureConnected()) return false;
    std::string key = std::string(kSessionPrefix) + user_id;
    redisReply* r = impl_->Command("DEL %s", key.c_str());
    if (!r) return false;
    freeReplyObject(r);
    return true;
}

static std::optional<UserSession> ParseUserSessionHash(redisReply* r, const std::string& user_id) {
    if (!r || r->type != REDIS_REPLY_ARRAY || r->elements == 0) return std::nullopt;
    UserSession s;
    s.user_id = user_id;
    for (size_t i = 0; i + 1 < r->elements; i += 2) {
        if (r->element[i]->type != REDIS_REPLY_STRING || r->element[i + 1]->type != REDIS_REPLY_STRING)
            continue;
        std::string field(r->element[i]->str, r->element[i]->len);
        std::string val(r->element[i + 1]->str, r->element[i + 1]->len);
        if (field == "gate_id") s.gate_id = val;
        else if (field == "gate_addr") s.gate_addr = val;
        else if (field == "device_type") s.device_type = val;
        else if (field == "device_id") s.device_id = val;
        else if (field == "online_at") { try { s.online_at = std::stoll(val); } catch (...) {} }
        else if (field == "last_active_at") { try { s.last_active_at = std::stoll(val); } catch (...) {} }
    }
    return s;
}

std::optional<UserSession> RedisSessionStore::GetSession(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->EnsureConnected()) return std::nullopt;
    std::string key = std::string(kSessionPrefix) + user_id;
    redisReply* r = impl_->Command("HGETALL %s", key.c_str());
    std::optional<UserSession> out = ParseUserSessionHash(r, user_id);
    if (r) freeReplyObject(r);
    return out;
}

std::vector<UserSession> RedisSessionStore::GetSessions(const std::vector<std::string>& user_ids) {
    std::vector<UserSession> result;
    for (const auto& uid : user_ids) {
        auto opt = GetSession(uid);
        if (opt) result.push_back(*opt);
    }
    return result;
}

bool RedisSessionStore::IsOnline(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->EnsureConnected()) return false;
    std::string key = std::string(kSessionPrefix) + user_id;
    redisReply* r = impl_->Command("EXISTS %s", key.c_str());
    bool ok = (r && r->type == REDIS_REPLY_INTEGER && r->integer == 1);
    if (r) freeReplyObject(r);
    return ok;
}

bool RedisSessionStore::UpdateLastActive(const std::string& user_id, int64_t timestamp) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->EnsureConnected()) return false;
    std::string key = std::string(kSessionPrefix) + user_id;
    redisReply* r = impl_->Command("HSET %s last_active_at %lld", key.c_str(), static_cast<long long>(timestamp));
    if (!r) return false;
    bool ok = true;
    freeReplyObject(r);
    r = impl_->Command("EXPIRE %s %d", key.c_str(), kSessionExpireSeconds);
    if (r) { ok = (r->type == REDIS_REPLY_INTEGER && r->integer == 1); freeReplyObject(r); }
    return ok;
}

bool RedisSessionStore::RegisterGate(const GateNode& node) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->EnsureConnected()) return false;
    std::string key = std::string(kGatePrefix) + node.gate_id;
    redisReply* r = impl_->Command("HMSET %s gate_id %s address %s current_connections %d registered_at %lld last_heartbeat %lld",
                                  key.c_str(), node.gate_id.c_str(), node.address.c_str(),
                                  node.current_connections,
                                  static_cast<long long>(node.registered_at),
                                  static_cast<long long>(node.last_heartbeat));
    if (!r) return false;
    bool ok = (r->type == REDIS_REPLY_STATUS || r->type == REDIS_REPLY_STRING);
    freeReplyObject(r);
    if (!ok) return false;
    r = impl_->Command("EXPIRE %s %d", key.c_str(), kGateExpireSeconds);
    if (r) { ok = (r->type == REDIS_REPLY_INTEGER && r->integer == 1); freeReplyObject(r); }
    if (!ok) return false;
    r = impl_->Command("SADD %s %s", kGateListKey, node.gate_id.c_str());
    if (r) { ok = (r->type == REDIS_REPLY_INTEGER); freeReplyObject(r); }
    return ok;
}

bool RedisSessionStore::UnregisterGate(const std::string& gate_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->EnsureConnected()) return false;
    std::string key = std::string(kGatePrefix) + gate_id;
    redisReply* r = impl_->Command("DEL %s", key.c_str());
    if (r) freeReplyObject(r);
    r = impl_->Command("SREM %s %s", kGateListKey, gate_id.c_str());
    if (r) freeReplyObject(r);
    return true;
}

bool RedisSessionStore::UpdateGateHeartbeat(const std::string& gate_id, int connections) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->EnsureConnected()) return false;
    int64_t now = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    std::string key = std::string(kGatePrefix) + gate_id;
    redisReply* r = impl_->Command("HSET %s current_connections %d last_heartbeat %lld", key.c_str(), connections, static_cast<long long>(now));
    if (!r) return false;
    freeReplyObject(r);
    r = impl_->Command("EXPIRE %s %d", key.c_str(), kGateExpireSeconds);
    bool ok = (r && r->type == REDIS_REPLY_INTEGER && r->integer == 1);
    if (r) freeReplyObject(r);
    return ok;
}

static std::optional<GateNode> ParseGateNodeHash(redisReply* r, const std::string& gate_id) {
    if (!r || r->type != REDIS_REPLY_ARRAY || r->elements == 0) return std::nullopt;
    GateNode node;
    node.gate_id = gate_id;
    for (size_t i = 0; i + 1 < r->elements; i += 2) {
        if (r->element[i]->type != REDIS_REPLY_STRING || r->element[i + 1]->type != REDIS_REPLY_STRING)
            continue;
        std::string field(r->element[i]->str, r->element[i]->len);
        std::string val(r->element[i + 1]->str, r->element[i + 1]->len);
        if (field == "address") node.address = val;
        else if (field == "current_connections") { try { node.current_connections = std::stoi(val); } catch (...) {} }
        else if (field == "registered_at") { try { node.registered_at = std::stoll(val); } catch (...) {} }
        else if (field == "last_heartbeat") { try { node.last_heartbeat = std::stoll(val); } catch (...) {} }
    }
    return node;
}

std::optional<GateNode> RedisSessionStore::GetGate(const std::string& gate_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->EnsureConnected()) return std::nullopt;
    std::string key = std::string(kGatePrefix) + gate_id;
    redisReply* r = impl_->Command("HGETALL %s", key.c_str());
    std::optional<GateNode> out = ParseGateNodeHash(r, gate_id);
    if (r) freeReplyObject(r);
    return out;
}

std::vector<GateNode> RedisSessionStore::GetAllGates() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->EnsureConnected()) return {};
    redisReply* r = impl_->Command("SMEMBERS %s", kGateListKey);
    if (!r || r->type != REDIS_REPLY_ARRAY) { if (r) freeReplyObject(r); return {}; }
    std::vector<GateNode> result;
    for (size_t i = 0; i < r->elements; i++) {
        if (r->element[i]->type != REDIS_REPLY_STRING) continue;
        std::string gid(r->element[i]->str, r->element[i]->len);
        std::string key = std::string(kGatePrefix) + gid;
        redisReply* h = impl_->Command("HGETALL %s", key.c_str());
        auto opt = ParseGateNodeHash(h, gid);
        if (h) freeReplyObject(h);
        if (opt) result.push_back(*opt);
    }
    freeReplyObject(r);
    return result;
}

#else  // !ZONESVR_USE_HIREDIS：无 hiredis 时的占位实现（需安装 libhiredis-dev 或 vcpkg hiredis 后重新配置并编译）

namespace {

void ParseRedisUrl(const std::string&, std::string* host, int* port) {
    *host = "127.0.0.1";
    *port = 6379;
}

}  // namespace

struct RedisSessionStore::Impl {
    std::string redis_url;
    std::string host;
    int port = 6379;
};

RedisSessionStore::RedisSessionStore(const std::string& redis_url)
    : impl_(std::make_unique<Impl>()) {
    impl_->redis_url = redis_url;
    ParseRedisUrl(redis_url, &impl_->host, &impl_->port);
}

RedisSessionStore::~RedisSessionStore() = default;

bool RedisSessionStore::SetOnline(const UserSession&) { (void)impl_; return false; }
bool RedisSessionStore::SetOffline(const std::string&) { return false; }
std::optional<UserSession> RedisSessionStore::GetSession(const std::string&) { return std::nullopt; }
std::vector<UserSession> RedisSessionStore::GetSessions(const std::vector<std::string>&) { return {}; }
bool RedisSessionStore::IsOnline(const std::string&) { return false; }
bool RedisSessionStore::UpdateLastActive(const std::string&, int64_t) { return false; }
bool RedisSessionStore::RegisterGate(const GateNode&) { return false; }
bool RedisSessionStore::UnregisterGate(const std::string&) { return false; }
bool RedisSessionStore::UpdateGateHeartbeat(const std::string&, int) { return false; }
std::optional<GateNode> RedisSessionStore::GetGate(const std::string&) { return std::nullopt; }
std::vector<GateNode> RedisSessionStore::GetAllGates() { return {}; }

#endif  // ZONESVR_USE_HIREDIS

}  // namespace swift::zone
