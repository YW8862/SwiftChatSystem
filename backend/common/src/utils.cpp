/**
 * @file utils.cpp
 * @brief 通用工具函数实现
 *
 * 哈希和编码使用 OpenSSL 库实现
 */

#include "swift/utils.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>

// OpenSSL 头文件
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

namespace swift {
namespace utils {

// ============================================================================
// 内部工具（匿名命名空间，仅本文件可见）
// ============================================================================
namespace {

/**
 * 获取线程安全的随机数生成器
 * 使用 thread_local 保证每个线程独立，避免锁竞争
 */
std::mt19937_64 &GetRandomEngine() {
  static thread_local std::random_device rd;
  static thread_local std::mt19937_64 gen(rd());
  return gen;
}

// 字母数字字符集（用于生成 ID）
const std::string kAlphaNum = "0123456789"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "abcdefghijklmnopqrstuvwxyz";

// 十六进制字符（小写）
const std::string kHexChars = "0123456789abcdef";

/**
 * 将字节数组转换为十六进制字符串
 */
std::string BytesToHex(const unsigned char *data, size_t len) {
  std::string result;
  result.reserve(len * 2);
  for (size_t i = 0; i < len; ++i) {
    result += kHexChars[(data[i] >> 4) & 0x0F];
    result += kHexChars[data[i] & 0x0F];
  }
  return result;
}

} // namespace

// ============================================================================
// ID 生成函数实现
// ============================================================================

std::string GenerateUUID() {
  auto &gen = GetRandomEngine();
  std::uniform_int_distribution<uint64_t> dis;

  // UUID v4 格式: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
  // 4 表示版本号，y 的高 2 位固定为 10（变体标识）
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  ss << std::setw(8) << (dis(gen) & 0xFFFFFFFF); // 8 位
  ss << "-";
  ss << std::setw(4) << (dis(gen) & 0xFFFF); // 4 位
  ss << "-";
  ss << std::setw(4) << ((dis(gen) & 0x0FFF) | 0x4000); // 4xxx（版本4）
  ss << "-";
  ss << std::setw(4) << ((dis(gen) & 0x3FFF) | 0x8000); // 变体标识
  ss << "-";
  ss << std::setw(12) << (dis(gen) & 0xFFFFFFFFFFFFULL); // 12 位

  return ss.str();
}

std::string GenerateShortId(const std::string &prefix, size_t length) {
  return prefix + RandomString(length);
}

std::string RandomString(size_t length, const std::string &charset) {
  // 使用传入的字符集，如果为空则使用默认字母数字
  const std::string &chars = charset.empty() ? kAlphaNum : charset;

  auto &gen = GetRandomEngine();
  std::uniform_int_distribution<size_t> dis(0, chars.size() - 1);

  std::string result;
  result.reserve(length);
  for (size_t i = 0; i < length; ++i) {
    result += chars[dis(gen)];
  }
  return result;
}

// ============================================================================
// 时间函数实现
// ============================================================================

int64_t GetTimestampMs() {
  auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             now.time_since_epoch())
      .count();
}

int64_t GetTimestampSec() {
  auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(
             now.time_since_epoch())
      .count();
}

int64_t GetTimestampUs() {
  auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(
             now.time_since_epoch())
      .count();
}

std::string FormatTimestamp(int64_t timestamp_ms, const std::string &format) {
  // 如果传入 0，使用当前时间
  if (timestamp_ms == 0) {
    timestamp_ms = GetTimestampMs();
  }

  // 转换为秒
  time_t seconds = timestamp_ms / 1000;
  struct tm tm_info;

  // 使用线程安全的 localtime_r（Linux）或 localtime_s（Windows）
#ifdef _WIN32
  localtime_s(&tm_info, &seconds);
#else
  localtime_r(&seconds, &tm_info);
#endif

  // 格式化输出
  char buffer[128];
  strftime(buffer, sizeof(buffer), format.c_str(), &tm_info);
  return std::string(buffer);
}

int64_t ParseTimestamp(const std::string &time_str, const std::string &format) {
  struct tm tm_info = {};

#ifdef _WIN32
  std::istringstream ss(time_str);
  ss >> std::get_time(&tm_info, format.c_str());
  if (ss.fail())
    return -1;
#else
  if (strptime(time_str.c_str(), format.c_str(), &tm_info) == nullptr) {
    return -1;
  }
#endif

  time_t seconds = mktime(&tm_info);
  if (seconds == -1)
    return -1;

  return static_cast<int64_t>(seconds) * 1000;
}

// ============================================================================
// 哈希函数实现 (OpenSSL)
// ============================================================================

std::string SHA256(const std::string &input) {
  unsigned char hash[SHA256_DIGEST_LENGTH]; // 32 字节

  // 计算 SHA256
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, input.c_str(), input.size());
  SHA256_Final(hash, &ctx);

  // 转换为十六进制字符串
  return BytesToHex(hash, SHA256_DIGEST_LENGTH);
}

std::string SHA512(const std::string &input) {
  unsigned char hash[SHA512_DIGEST_LENGTH]; // 64 字节

  SHA512_CTX ctx;
  SHA512_Init(&ctx);
  SHA512_Update(&ctx, input.c_str(), input.size());
  SHA512_Final(hash, &ctx);

  return BytesToHex(hash, SHA512_DIGEST_LENGTH);
}

std::string MD5(const std::string &input) {
  unsigned char hash[MD5_DIGEST_LENGTH]; // 16 字节

  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, input.c_str(), input.size());
  MD5_Final(hash, &ctx);

  return BytesToHex(hash, MD5_DIGEST_LENGTH);
}

std::string HmacSHA256(const std::string &key, const std::string &data) {
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hash_len = 0;

  // 使用 HMAC 计算签名
  HMAC(EVP_sha256(), key.c_str(), static_cast<int>(key.size()),
       reinterpret_cast<const unsigned char *>(data.c_str()), data.size(), hash,
       &hash_len);

  return BytesToHex(hash, hash_len);
}

// ============================================================================
// 编码函数实现 (OpenSSL)
// ============================================================================

std::string Base64Encode(const std::string &input) {
  return Base64Encode(reinterpret_cast<const uint8_t *>(input.data()),
                      input.size());
}

std::string Base64Encode(const uint8_t *data, size_t len) {
  // 创建 BIO 链：base64 过滤器 -> 内存缓冲区
  BIO *b64 = BIO_new(BIO_f_base64());
  BIO *mem = BIO_new(BIO_s_mem());
  b64 = BIO_push(b64, mem);

  // 设置不换行（默认每 64 字符换行）
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

  // 写入数据
  BIO_write(b64, data, static_cast<int>(len));
  BIO_flush(b64);

  // 获取结果
  BUF_MEM *buf;
  BIO_get_mem_ptr(b64, &buf);
  std::string result(buf->data, buf->length);

  // 释放资源
  BIO_free_all(b64);

  return result;
}

std::string Base64Decode(const std::string &input) {
  // 创建 BIO 链
  BIO *b64 = BIO_new(BIO_f_base64());
  BIO *mem = BIO_new_mem_buf(input.data(), static_cast<int>(input.size()));
  mem = BIO_push(b64, mem);

  BIO_set_flags(mem, BIO_FLAGS_BASE64_NO_NL);

  // 读取解码后的数据
  std::string result(input.size(), '\0');
  int len = BIO_read(mem, &result[0], static_cast<int>(input.size()));

  BIO_free_all(mem);

  if (len > 0) {
    result.resize(len);
  } else {
    result.clear();
  }
  return result;
}

std::string UrlEncode(const std::string &input) {
  std::ostringstream escaped;
  escaped << std::hex << std::setfill('0');

  for (unsigned char c : input) {
    // 保留字母、数字和 -_.~ 不编码
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
    } else {
      // 其他字符转为 %XX 格式
      escaped << '%' << std::setw(2) << static_cast<int>(c);
    }
  }
  return escaped.str();
}

std::string UrlDecode(const std::string &input) {
  std::string result;
  result.reserve(input.size());

  for (size_t i = 0; i < input.size(); ++i) {
    if (input[i] == '%' && i + 2 < input.size()) {
      // 解码 %XX
      int val;
      std::istringstream iss(input.substr(i + 1, 2));
      if (iss >> std::hex >> val) {
        result += static_cast<char>(val);
        i += 2;
        continue;
      }
    } else if (input[i] == '+') {
      // + 解码为空格
      result += ' ';
      continue;
    }
    result += input[i];
  }
  return result;
}

std::string HexEncode(const std::string &input) {
  return HexEncode(reinterpret_cast<const uint8_t *>(input.data()),
                   input.size());
}

std::string HexEncode(const uint8_t *data, size_t len) {
  return BytesToHex(data, len);
}

std::string HexDecode(const std::string &input) {
  std::string result;
  result.reserve(input.size() / 2);

  for (size_t i = 0; i + 1 < input.size(); i += 2) {
    // 将两个十六进制字符转为一个字节
    char high = input[i];
    char low = input[i + 1];

    int high_val =
        std::isdigit(high) ? high - '0' : std::tolower(high) - 'a' + 10;
    int low_val = std::isdigit(low) ? low - '0' : std::tolower(low) - 'a' + 10;

    result += static_cast<char>((high_val << 4) | low_val);
  }
  return result;
}

// ============================================================================
// 字符串处理函数实现
// ============================================================================

std::vector<std::string> Split(const std::string &str, char delimiter) {
  std::vector<std::string> result;
  std::stringstream ss(str);
  std::string item;

  while (std::getline(ss, item, delimiter)) {
    result.push_back(item);
  }
  return result;
}

std::vector<std::string> Split(const std::string &str,
                               const std::string &delimiter) {
  std::vector<std::string> result;

  if (delimiter.empty()) {
    result.push_back(str);
    return result;
  }

  size_t start = 0;
  size_t end;

  while ((end = str.find(delimiter, start)) != std::string::npos) {
    result.push_back(str.substr(start, end - start));
    start = end + delimiter.size();
  }
  result.push_back(str.substr(start));

  return result;
}

std::string Join(const std::vector<std::string> &parts,
                 const std::string &delimiter) {
  std::string result;

  for (size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      result += delimiter;
    }
    result += parts[i];
  }
  return result;
}

std::string Trim(const std::string &str) { return TrimLeft(TrimRight(str)); }

std::string TrimLeft(const std::string &str) {
  size_t start = str.find_first_not_of(" \t\n\r\f\v");
  return (start == std::string::npos) ? "" : str.substr(start);
}

std::string TrimRight(const std::string &str) {
  size_t end = str.find_last_not_of(" \t\n\r\f\v");
  return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

std::string ToLower(const std::string &str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

std::string ToUpper(const std::string &str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return result;
}

std::string Replace(const std::string &str, const std::string &from,
                    const std::string &to) {
  if (from.empty())
    return str;

  size_t pos = str.find(from);
  if (pos == std::string::npos)
    return str;

  std::string result = str;
  result.replace(pos, from.size(), to);
  return result;
}

std::string ReplaceAll(const std::string &str, const std::string &from,
                       const std::string &to) {
  if (from.empty())
    return str;

  std::string result = str;
  size_t pos = 0;

  while ((pos = result.find(from, pos)) != std::string::npos) {
    result.replace(pos, from.size(), to);
    pos += to.size();
  }
  return result;
}

bool StartsWith(const std::string &str, const std::string &prefix) {
  if (prefix.size() > str.size())
    return false;
  return str.compare(0, prefix.size(), prefix) == 0;
}

bool EndsWith(const std::string &str, const std::string &suffix) {
  if (suffix.size() > str.size())
    return false;
  return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool IsDigit(const std::string &str) {
  if (str.empty())
    return false;
  return std::all_of(str.begin(), str.end(),
                     [](unsigned char c) { return std::isdigit(c); });
}

bool IsEmpty(const std::string &str) { return str.empty(); }

bool IsBlank(const std::string &str) {
  return std::all_of(str.begin(), str.end(),
                     [](unsigned char c) { return std::isspace(c); });
}

// ============================================================================
// 类型转换函数实现
// ============================================================================

int32_t ToInt32(const std::string &str, int32_t default_val) {
  try {
    return std::stoi(str);
  } catch (...) {
    return default_val;
  }
}

int64_t ToInt64(const std::string &str, int64_t default_val) {
  try {
    return std::stoll(str);
  } catch (...) {
    return default_val;
  }
}

double ToDouble(const std::string &str, double default_val) {
  try {
    return std::stod(str);
  } catch (...) {
    return default_val;
  }
}

} // namespace utils
} // namespace swift
