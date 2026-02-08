#include "config.h"
#include "swift/config_loader.h"

namespace swift::online {

OnlineConfig LoadConfig(const std::string& config_file) {
    auto kv = swift::LoadKeyValueConfig(config_file, "ONLINESVR_");

    OnlineConfig c;
    c.host = kv.Get("host", c.host);
    c.port = kv.GetInt("port", c.port);
    c.store_type = kv.Get("store_type", c.store_type);
    c.rocksdb_path = kv.Get("rocksdb_path", c.rocksdb_path);
    c.jwt_secret = kv.Get("jwt_secret", c.jwt_secret);
    c.jwt_expire_hours = kv.GetInt("jwt_expire_hours", c.jwt_expire_hours);
    c.log_dir = kv.Get("log_dir", c.log_dir);
    c.log_level = kv.Get("log_level", c.log_level);
    return c;
}

}  // namespace swift::online
