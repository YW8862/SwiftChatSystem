#include "config.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace swift::chat {

namespace {

void Trim(std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        s.clear();
        return;
    }
    auto end = s.find_last_not_of(" \t\r\n");
    s = s.substr(start, end == std::string::npos ? std::string::npos : end - start + 1);
}

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

int ParseInt(const std::string& value, int default_val) {
    try {
        return std::stoi(value);
    } catch (...) {
        return default_val;
    }
}

void ApplyEnvOverrides(ChatConfig& config) {
    const char* p;

    if ((p = std::getenv("CHATSVR_HOST")) != nullptr && p[0] != '\0')
        config.host = p;
    if ((p = std::getenv("CHATSVR_PORT")) != nullptr && p[0] != '\0')
        config.port = ParseInt(p, config.port);
    if ((p = std::getenv("CHATSVR_STORE_TYPE")) != nullptr && p[0] != '\0')
        config.store_type = p;
    if ((p = std::getenv("CHATSVR_ROCKSDB_PATH")) != nullptr && p[0] != '\0')
        config.rocksdb_path = p;
    if ((p = std::getenv("CHATSVR_MYSQL_DSN")) != nullptr && p[0] != '\0')
        config.mysql_dsn = p;

    if ((p = std::getenv("CHATSVR_RECALL_TIMEOUT")) != nullptr && p[0] != '\0')
        config.recall_timeout_seconds = ParseInt(p, config.recall_timeout_seconds);
    if ((p = std::getenv("CHATSVR_OFFLINE_MAX")) != nullptr && p[0] != '\0')
        config.offline_max_count = ParseInt(p, config.offline_max_count);
    if ((p = std::getenv("CHATSVR_HISTORY_PAGE_SIZE")) != nullptr && p[0] != '\0')
        config.history_page_size = ParseInt(p, config.history_page_size);

    if ((p = std::getenv("CHATSVR_LOG_DIR")) != nullptr && p[0] != '\0')
        config.log_dir = p;
    if ((p = std::getenv("CHATSVR_LOG_LEVEL")) != nullptr && p[0] != '\0')
        config.log_level = p;
}

void ParseLine(const std::string& line, ChatConfig& config) {
    std::string key, value;
    size_t eq = line.find('=');
    if (eq == std::string::npos)
        return;
    key = line.substr(0, eq);
    value = line.substr(eq + 1);
    Trim(key);
    Trim(value);
    if (key.empty())
        return;

    std::string k = ToLower(key);
    if (k == "host")
        config.host = value;
    else if (k == "port")
        config.port = ParseInt(value, config.port);
    else if (k == "store_type")
        config.store_type = value;
    else if (k == "rocksdb_path")
        config.rocksdb_path = value;
    else if (k == "mysql_dsn")
        config.mysql_dsn = value;
    else if (k == "recall_timeout_seconds")
        config.recall_timeout_seconds = ParseInt(value, config.recall_timeout_seconds);
    else if (k == "offline_max_count")
        config.offline_max_count = ParseInt(value, config.offline_max_count);
    else if (k == "history_page_size")
        config.history_page_size = ParseInt(value, config.history_page_size);
    else if (k == "log_dir")
        config.log_dir = value;
    else if (k == "log_level")
        config.log_level = value;
}

}  // namespace

ChatConfig LoadConfig(const std::string& config_file) {
    ChatConfig config;

    std::ifstream f(config_file);
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            Trim(line);
            if (line.empty() || line[0] == '#')
                continue;
            ParseLine(line, config);
        }
    }

    ApplyEnvOverrides(config);
    return config;
}

}  // namespace swift::chat

