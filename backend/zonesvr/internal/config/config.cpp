#include "config.h"
#include "swift/config_loader.h"

namespace swift::zone {

ZoneConfig LoadConfig(const std::string& config_file) {
    auto kv = swift::LoadKeyValueConfig(config_file, "ZONESVR_");
    ZoneConfig c;
    c.host = kv.Get("host", c.host);
    c.port = kv.GetInt("port", c.port);
    c.auth_svr_addr = kv.Get("auth_svr_addr", c.auth_svr_addr);
    c.online_svr_addr = kv.Get("online_svr_addr", c.online_svr_addr);
    c.friend_svr_addr = kv.Get("friend_svr_addr", c.friend_svr_addr);
    c.chat_svr_addr = kv.Get("chat_svr_addr", c.chat_svr_addr);
    c.file_svr_addr = kv.Get("file_svr_addr", c.file_svr_addr);
    c.gate_svr_addr = kv.Get("gate_svr_addr", c.gate_svr_addr);
    c.session_store_type = kv.Get("session_store_type", c.session_store_type);
    c.redis_url = kv.Get("redis_url", c.redis_url);
    c.session_expire_seconds = kv.GetInt("session_expire_seconds", c.session_expire_seconds);
    c.gate_heartbeat_timeout = kv.GetInt("gate_heartbeat_timeout", c.gate_heartbeat_timeout);
    c.log_dir = kv.Get("log_dir", c.log_dir);
    c.log_level = kv.Get("log_level", c.log_level);
    c.internal_secret = kv.Get("internal_secret", c.internal_secret);
    return c;
}

}  // namespace swift::zone
