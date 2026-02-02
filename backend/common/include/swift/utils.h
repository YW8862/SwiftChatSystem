#pragma once

#include <string>
#include <chrono>

namespace swift {
namespace utils {

/**
 * 生成 UUID
 */
std::string GenerateUUID();

/**
 * 获取当前时间戳（毫秒）
 */
int64_t GetTimestampMs();

/**
 * 获取当前时间戳（秒）
 */
int64_t GetTimestampSec();

/**
 * 时间戳格式化
 */
std::string FormatTimestamp(int64_t timestamp_ms, const std::string& format = "%Y-%m-%d %H:%M:%S");

/**
 * 字符串哈希
 */
std::string SHA256(const std::string& input);

/**
 * Base64 编解码
 */
std::string Base64Encode(const std::string& input);
std::string Base64Decode(const std::string& input);

}  // namespace utils
}  // namespace swift
