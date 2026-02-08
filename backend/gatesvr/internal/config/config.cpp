#include "config.h"
#include "swift/config_loader.h"
#include <cstdlib>

namespace swift::gate {

GateConfig LoadConfig(const std::string& config_file) {
    auto kv = swift::LoadKeyValueConfig(config_file, "GATESVR_");
    GateConfig c;
    c.host = kv.Get("host", c.host);
    c.websocket_port = kv.GetInt("websocket_port", c.websocket_port);
    c.grpc_port = kv.GetInt("grpc_port", c.grpc_port);
    c.gate_id = kv.Get("gate_id", c.gate_id);
    c.zone_svr_addr = kv.Get("zone_svr_addr", c.zone_svr_addr);
    c.max_connections = kv.GetInt("max_connections", c.max_connections);
    c.heartbeat_interval_seconds = kv.GetInt("heartbeat_interval_seconds", c.heartbeat_interval_seconds);
    c.heartbeat_timeout_seconds = kv.GetInt("heartbeat_timeout_seconds", c.heartbeat_timeout_seconds);
    c.log_dir = kv.Get("log_dir", c.log_dir);
    c.log_level = kv.Get("log_level", c.log_level);
    // zonesvr_internal_secret：环境变量 GATESVR_ZONESVR_INTERNAL_SECRET 覆盖（ApplyEnvOverrides 已将 GATESVR_ZONESVR_INTERNAL_SECRET -> zonesvr_internal_secret）
    c.zonesvr_internal_secret = kv.Get("zonesvr_internal_secret", c.zonesvr_internal_secret);
    return c;
}

}  // namespace swift::gate
