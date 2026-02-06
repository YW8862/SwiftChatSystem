#pragma once

/**
 * @file config_loader.h
 * @brief 通用 key=value 配置加载：文件解析 + 环境变量覆盖，配置项不写死在代码里。
 *
 * 用法：
 *   KeyValueConfig cfg;
 *   cfg.LoadFile("filesvr.conf");
 *   cfg.ApplyEnvOverrides("FILESVR_");
 *   int port = cfg.GetInt("grpc_port", 9100);
 *
 * 或一行： auto cfg = LoadKeyValueConfig("filesvr.conf", "FILESVR_");
 */

#include <cstdint>
#include <string>
#include <unordered_map>

namespace swift {

/**
 * 键值对配置：键统一小写，值来自配置文件与环境变量（环境变量覆盖文件）。
 * 配置参数完全由文件/环境提供，无需在代码里写死默认值（Get 的 default 仅当未配置时使用）。
 */
class KeyValueConfig {
public:
    KeyValueConfig() = default;

    /**
     * 从 key=value 文件加载（支持 # 注释、空行、trim）。
     * 键会转为小写存储。
     */
    void LoadFile(const std::string& path);

    /**
     * 用环境变量覆盖：只处理 NAME_PREFIX_* 的变量，去掉前缀并转小写后作为 key。
     * 例如 prefix="FILESVR_" 时，FILESVR_GRPC_PORT=9101 -> grpc_port=9101
     */
    void ApplyEnvOverrides(const std::string& env_prefix);

    /** 一次完成：LoadFile(path) + ApplyEnvOverrides(env_prefix) */
    void Load(const std::string& path, const std::string& env_prefix);

    bool Has(const std::string& key) const;

    std::string Get(const std::string& key, const std::string& default_val) const;
    int GetInt(const std::string& key, int default_val) const;
    int64_t GetInt64(const std::string& key, int64_t default_val) const;
    bool GetBool(const std::string& key, bool default_val) const;

    /** 原始 map，便于需要遍历或批量填充 struct 的场景 */
    const std::unordered_map<std::string, std::string>& Data() const { return map_; }

private:
    static std::string ToLowerKey(std::string s);
    std::unordered_map<std::string, std::string> map_;
};

/**
 * 加载配置：读文件并应用环境变量覆盖，返回 KeyValueConfig。
 */
KeyValueConfig LoadKeyValueConfig(const std::string& path,
                                 const std::string& env_prefix);

}  // namespace swift
