#pragma once

/**
 * @file utils.h
 * @brief 通用工具函数库
 * 
 * 提供 ID 生成、时间处理、哈希编码、字符串操作等常用功能
 * 
 * @example
 *   #include <swift/utils.h>
 *   
 *   // 生成用户 ID
 *   std::string user_id = swift::utils::GenerateShortId("u_");  // "u_7kX9mPqR3sT1"
 *   
 *   // 密码哈希
 *   std::string hash = swift::utils::SHA256("password123");
 *   
 *   // 获取当前时间
 *   int64_t now = swift::utils::GetTimestampMs();
 *   std::string time_str = swift::utils::FormatTimestamp(now);  // "2026-02-03 00:30:00"
 */

#include <string>
#include <vector>
#include <cstdint>

namespace swift {
namespace utils {

// ============================================================================
// UUID / ID 生成
// ============================================================================

/**
 * @brief 生成 UUID v4
 * @return 36 字符的 UUID 字符串
 * 
 * @example
 *   std::string uuid = GenerateUUID();
 *   // 返回: "a1b2c3d4-e5f6-4789-abcd-ef0123456789"
 */
std::string GenerateUUID();

/**
 * @brief 生成带前缀的短 ID（适用于用户ID、消息ID等）
 * @param prefix ID 前缀，如 "u_"(用户), "m_"(消息), "c_"(会话)
 * @param length ID 长度（不含前缀），默认 12
 * @return 格式: prefix + 随机字符串
 * 
 * @example
 *   std::string user_id = GenerateShortId("u_");      // "u_7kX9mPqR3sT1"
 *   std::string msg_id = GenerateShortId("m_", 16);   // "m_Ab3Cd5Ef7Gh9Ij1K"
 */
std::string GenerateShortId(const std::string& prefix = "", size_t length = 12);

/**
 * @brief 生成随机字符串
 * @param length 字符串长度
 * @param charset 字符集，空则使用默认字母数字
 * @return 随机字符串
 * 
 * @example
 *   std::string token = RandomString(32);                    // 32位随机令牌
 *   std::string code = RandomString(6, "0123456789");        // 6位数字验证码
 */
std::string RandomString(size_t length, const std::string& charset = "");

// ============================================================================
// 时间函数
// ============================================================================

/**
 * @brief 获取当前时间戳（毫秒）
 * @return Unix 时间戳（毫秒）
 * 
 * @example
 *   int64_t now = GetTimestampMs();  // 1706918400000
 */
int64_t GetTimestampMs();

/**
 * @brief 获取当前时间戳（秒）
 * @return Unix 时间戳（秒）
 * 
 * @example
 *   int64_t now = GetTimestampSec();  // 1706918400
 */
int64_t GetTimestampSec();

/**
 * @brief 获取当前时间戳（微秒）
 * @return Unix 时间戳（微秒）
 * 
 * @example
 *   int64_t now = GetTimestampUs();  // 1706918400000000
 */
int64_t GetTimestampUs();

/**
 * @brief 格式化时间戳为字符串
 * @param timestamp_ms 毫秒时间戳，0 表示当前时间
 * @param format 格式字符串，支持 strftime 格式
 * @return 格式化后的时间字符串
 * 
 * @example
 *   std::string t1 = FormatTimestamp();                           // "2026-02-03 00:30:00"
 *   std::string t2 = FormatTimestamp(0, "%Y/%m/%d");              // "2026/02/03"
 *   std::string t3 = FormatTimestamp(1706918400000, "%H:%M:%S");  // "00:00:00"
 */
std::string FormatTimestamp(int64_t timestamp_ms = 0, const std::string& format = "%Y-%m-%d %H:%M:%S");

/**
 * @brief 解析时间字符串为时间戳
 * @param time_str 时间字符串
 * @param format 格式字符串
 * @return 毫秒时间戳，解析失败返回 -1
 * 
 * @example
 *   int64_t ts = ParseTimestamp("2026-02-03 12:00:00");  // 返回对应的毫秒时间戳
 */
int64_t ParseTimestamp(const std::string& time_str, const std::string& format = "%Y-%m-%d %H:%M:%S");

// ============================================================================
// 哈希函数 (使用 OpenSSL)
// ============================================================================

/**
 * @brief 计算 SHA256 哈希值
 * @param input 输入字符串
 * @return 64 字符的十六进制哈希值
 * 
 * @example
 *   std::string hash = SHA256("hello");
 *   // 返回: "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"
 *   
 *   // 密码加盐哈希
 *   std::string password_hash = SHA256(password + salt);
 */
std::string SHA256(const std::string& input);

/**
 * @brief 计算 SHA512 哈希值
 * @param input 输入字符串
 * @return 128 字符的十六进制哈希值
 * 
 * @example
 *   std::string hash = SHA512("hello");
 */
std::string SHA512(const std::string& input);

/**
 * @brief 计算 MD5 哈希值（不推荐用于安全场景）
 * @param input 输入字符串
 * @return 32 字符的十六进制哈希值
 * 
 * @example
 *   std::string hash = MD5("hello");
 *   // 返回: "5d41402abc4b2a76b9719d911017c592"
 *   
 *   // 常用于文件校验、缓存 key
 *   std::string file_hash = MD5(file_content);
 */
std::string MD5(const std::string& input);

/**
 * @brief 计算 HMAC-SHA256 签名
 * @param key 密钥
 * @param data 待签名数据
 * @return 64 字符的十六进制签名
 * 
 * @example
 *   std::string sig = HmacSHA256("secret_key", "message");
 *   // 用于 API 签名、JWT 等
 */
std::string HmacSHA256(const std::string& key, const std::string& data);

// ============================================================================
// 编码函数
// ============================================================================

/**
 * @brief Base64 编码
 * @param input 输入字符串
 * @return Base64 编码后的字符串
 * 
 * @example
 *   std::string encoded = Base64Encode("hello");  // "aGVsbG8="
 */
std::string Base64Encode(const std::string& input);
std::string Base64Encode(const uint8_t* data, size_t len);

/**
 * @brief Base64 解码
 * @param input Base64 编码的字符串
 * @return 解码后的原始字符串
 * 
 * @example
 *   std::string decoded = Base64Decode("aGVsbG8=");  // "hello"
 */
std::string Base64Decode(const std::string& input);

/**
 * @brief URL 编码（百分号编码）
 * @param input 输入字符串
 * @return URL 编码后的字符串
 * 
 * @example
 *   std::string encoded = UrlEncode("hello world");  // "hello%20world"
 *   std::string encoded = UrlEncode("a=1&b=2");      // "a%3D1%26b%3D2"
 */
std::string UrlEncode(const std::string& input);

/**
 * @brief URL 解码
 * @param input URL 编码的字符串
 * @return 解码后的原始字符串
 * 
 * @example
 *   std::string decoded = UrlDecode("hello%20world");  // "hello world"
 */
std::string UrlDecode(const std::string& input);

/**
 * @brief 十六进制编码
 * @param input 输入字符串或字节数组
 * @return 十六进制字符串（小写）
 * 
 * @example
 *   std::string hex = HexEncode("AB");  // "4142"
 */
std::string HexEncode(const std::string& input);
std::string HexEncode(const uint8_t* data, size_t len);

/**
 * @brief 十六进制解码
 * @param input 十六进制字符串
 * @return 解码后的原始字符串
 * 
 * @example
 *   std::string raw = HexDecode("4142");  // "AB"
 */
std::string HexDecode(const std::string& input);

// ============================================================================
// 字符串处理函数
// ============================================================================

/**
 * @brief 按分隔符分割字符串
 * @param str 输入字符串
 * @param delimiter 分隔符（字符或字符串）
 * @return 分割后的字符串数组
 * 
 * @example
 *   auto parts = Split("a,b,c", ',');       // ["a", "b", "c"]
 *   auto parts = Split("a::b::c", "::");    // ["a", "b", "c"]
 */
std::vector<std::string> Split(const std::string& str, char delimiter);
std::vector<std::string> Split(const std::string& str, const std::string& delimiter);

/**
 * @brief 连接字符串数组
 * @param parts 字符串数组
 * @param delimiter 连接符
 * @return 连接后的字符串
 * 
 * @example
 *   std::string s = Join({"a", "b", "c"}, ",");  // "a,b,c"
 */
std::string Join(const std::vector<std::string>& parts, const std::string& delimiter);

/**
 * @brief 去除首尾空白字符
 * @param str 输入字符串
 * @return 去除空白后的字符串
 * 
 * @example
 *   std::string s = Trim("  hello  ");  // "hello"
 */
std::string Trim(const std::string& str);
std::string TrimLeft(const std::string& str);   // 只去除左侧空白
std::string TrimRight(const std::string& str);  // 只去除右侧空白

/**
 * @brief 转换为小写
 * @example
 *   std::string s = ToLower("HELLO");  // "hello"
 */
std::string ToLower(const std::string& str);

/**
 * @brief 转换为大写
 * @example
 *   std::string s = ToUpper("hello");  // "HELLO"
 */
std::string ToUpper(const std::string& str);

/**
 * @brief 替换字符串（只替换第一个匹配）
 * @example
 *   std::string s = Replace("hello", "l", "L");  // "heLlo"
 */
std::string Replace(const std::string& str, const std::string& from, const std::string& to);

/**
 * @brief 替换所有匹配的字符串
 * @example
 *   std::string s = ReplaceAll("hello", "l", "L");  // "heLLo"
 */
std::string ReplaceAll(const std::string& str, const std::string& from, const std::string& to);

/**
 * @brief 判断是否以指定前缀开头
 * @example
 *   bool b = StartsWith("hello", "he");  // true
 */
bool StartsWith(const std::string& str, const std::string& prefix);

/**
 * @brief 判断是否以指定后缀结尾
 * @example
 *   bool b = EndsWith("hello.txt", ".txt");  // true
 */
bool EndsWith(const std::string& str, const std::string& suffix);

/**
 * @brief 判断字符串是否全为数字
 * @example
 *   bool b = IsDigit("12345");  // true
 *   bool b = IsDigit("123a5");  // false
 */
bool IsDigit(const std::string& str);

/**
 * @brief 判断字符串是否为空
 * @example
 *   bool b = IsEmpty("");  // true
 */
bool IsEmpty(const std::string& str);

/**
 * @brief 判断字符串是否为空或只有空白字符
 * @example
 *   bool b = IsBlank("   ");  // true
 *   bool b = IsBlank("  a ");  // false
 */
bool IsBlank(const std::string& str);

// ============================================================================
// 类型转换函数
// ============================================================================

/**
 * @brief 字符串转 int32（失败返回默认值）
 * @example
 *   int32_t n = ToInt32("123");        // 123
 *   int32_t n = ToInt32("abc", -1);    // -1
 */
int32_t ToInt32(const std::string& str, int32_t default_val = 0);

/**
 * @brief 字符串转 int64（失败返回默认值）
 * @example
 *   int64_t n = ToInt64("9999999999");  // 9999999999
 */
int64_t ToInt64(const std::string& str, int64_t default_val = 0);

/**
 * @brief 字符串转 double（失败返回默认值）
 * @example
 *   double d = ToDouble("3.14");  // 3.14
 */
double ToDouble(const std::string& str, double default_val = 0.0);

}  // namespace utils
}  // namespace swift
