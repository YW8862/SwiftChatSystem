/**
 * @file test_utils.cpp
 * @brief utils 工具函数测试
 * 
 * 编译: make test_utils
 * 运行: ./bin/test_utils
 */

#include <swift/utils.h>
#include <iostream>
#include <cassert>

using namespace swift::utils;

// 测试通过打印绿色，失败打印红色
#define TEST(name) std::cout << "\n[TEST] " << name << std::endl
#define PASS(msg) std::cout << "  \033[32m✓\033[0m " << msg << std::endl
#define FAIL(msg) std::cout << "  \033[31m✗\033[0m " << msg << std::endl; failed++
#define CHECK(cond, msg) if (cond) { PASS(msg); } else { FAIL(msg); }

int main() {
    int failed = 0;
    
    std::cout << "========================================" << std::endl;
    std::cout << "       Swift Utils 功能测试" << std::endl;
    std::cout << "========================================" << std::endl;

    // ========================================
    // 1. ID 生成测试
    // ========================================
    TEST("ID 生成");
    {
        // UUID
        std::string uuid = GenerateUUID();
        std::cout << "  UUID: " << uuid << std::endl;
        CHECK(uuid.length() == 36, "UUID 长度正确 (36)");
        CHECK(uuid[8] == '-' && uuid[13] == '-', "UUID 格式正确");
        
        // 短 ID
        std::string user_id = GenerateShortId("u_");
        std::cout << "  用户ID: " << user_id << std::endl;
        CHECK(user_id.substr(0, 2) == "u_", "用户ID 前缀正确");
        CHECK(user_id.length() == 14, "用户ID 长度正确 (2+12)");
        
        std::string msg_id = GenerateShortId("m_", 16);
        std::cout << "  消息ID: " << msg_id << std::endl;
        CHECK(msg_id.length() == 18, "消息ID 长度正确 (2+16)");
        
        // 验证码
        std::string code = RandomString(6, "0123456789");
        std::cout << "  验证码: " << code << std::endl;
        CHECK(code.length() == 6, "验证码长度正确");
        CHECK(IsDigit(code), "验证码全是数字");
    }

    // ========================================
    // 2. 时间函数测试
    // ========================================
    TEST("时间函数");
    {
        int64_t ts_ms = GetTimestampMs();
        int64_t ts_sec = GetTimestampSec();
        int64_t ts_us = GetTimestampUs();
        
        std::cout << "  毫秒时间戳: " << ts_ms << std::endl;
        std::cout << "  秒时间戳:   " << ts_sec << std::endl;
        std::cout << "  微秒时间戳: " << ts_us << std::endl;
        
        CHECK(ts_ms > 0, "毫秒时间戳 > 0");
        CHECK(ts_ms / 1000 == ts_sec, "毫秒/1000 == 秒");
        CHECK(ts_us / 1000 == ts_ms, "微秒/1000 == 毫秒");
        
        // 格式化
        std::string formatted = FormatTimestamp(ts_ms);
        std::cout << "  格式化时间: " << formatted << std::endl;
        CHECK(formatted.length() == 19, "格式化时间长度正确");
        
        std::string date_only = FormatTimestamp(ts_ms, "%Y-%m-%d");
        std::cout << "  仅日期: " << date_only << std::endl;
        CHECK(date_only.length() == 10, "日期格式正确");
        
        // 解析
        int64_t parsed = ParseTimestamp(formatted);
        CHECK(parsed > 0, "时间解析成功");
        CHECK(std::abs(parsed - ts_ms) < 1000, "解析精度在1秒内");
    }

    // ========================================
    // 3. 哈希函数测试 (OpenSSL)
    // ========================================
    TEST("哈希函数 (OpenSSL)");
    {
        // SHA256 - 已知结果验证
        std::string sha256_result = SHA256("hello");
        std::cout << "  SHA256(hello): " << sha256_result << std::endl;
        CHECK(sha256_result == "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824",
              "SHA256 结果正确");
        CHECK(sha256_result.length() == 64, "SHA256 长度正确 (64)");
        
        // SHA512
        std::string sha512_result = SHA512("hello");
        std::cout << "  SHA512(hello): " << sha512_result.substr(0, 32) << "..." << std::endl;
        CHECK(sha512_result.length() == 128, "SHA512 长度正确 (128)");
        
        // MD5 - 已知结果验证
        std::string md5_result = MD5("hello");
        std::cout << "  MD5(hello): " << md5_result << std::endl;
        CHECK(md5_result == "5d41402abc4b2a76b9719d911017c592", "MD5 结果正确");
        CHECK(md5_result.length() == 32, "MD5 长度正确 (32)");
        
        // HMAC-SHA256
        std::string hmac_result = HmacSHA256("secret", "message");
        std::cout << "  HMAC-SHA256: " << hmac_result << std::endl;
        CHECK(hmac_result.length() == 64, "HMAC-SHA256 长度正确");
        
        // 密码哈希示例
        std::string password = "mypassword";
        std::string salt = "random_salt_123";
        std::string password_hash = SHA256(password + salt);
        std::cout << "  密码哈希: " << password_hash.substr(0, 16) << "..." << std::endl;
        CHECK(!password_hash.empty(), "密码哈希成功");
    }

    // ========================================
    // 4. 编码函数测试 (OpenSSL)
    // ========================================
    TEST("编码函数 (OpenSSL)");
    {
        // Base64
        std::string b64_encoded = Base64Encode("hello world");
        std::cout << "  Base64 编码: " << b64_encoded << std::endl;
        CHECK(b64_encoded == "aGVsbG8gd29ybGQ=", "Base64 编码正确");
        
        std::string b64_decoded = Base64Decode(b64_encoded);
        std::cout << "  Base64 解码: " << b64_decoded << std::endl;
        CHECK(b64_decoded == "hello world", "Base64 解码正确");
        
        // URL 编码
        std::string url_encoded = UrlEncode("hello world&foo=bar");
        std::cout << "  URL 编码: " << url_encoded << std::endl;
        CHECK(url_encoded == "hello%20world%26foo%3dbar", "URL 编码正确");
        
        std::string url_decoded = UrlDecode(url_encoded);
        CHECK(url_decoded == "hello world&foo=bar", "URL 解码正确");
        
        // Hex 编码
        std::string hex_encoded = HexEncode("ABC");
        std::cout << "  Hex 编码: " << hex_encoded << std::endl;
        CHECK(hex_encoded == "414243", "Hex 编码正确");
        
        std::string hex_decoded = HexDecode(hex_encoded);
        CHECK(hex_decoded == "ABC", "Hex 解码正确");
    }

    // ========================================
    // 5. 字符串处理测试
    // ========================================
    TEST("字符串处理");
    {
        // Split
        auto parts = Split("a,b,c", ',');
        CHECK(parts.size() == 3, "Split 分割数量正确");
        CHECK(parts[0] == "a" && parts[1] == "b" && parts[2] == "c", "Split 内容正确");
        
        auto parts2 = Split("a::b::c", "::");
        CHECK(parts2.size() == 3, "Split 字符串分隔符正确");
        
        // Join
        std::string joined = Join({"x", "y", "z"}, "-");
        std::cout << "  Join: " << joined << std::endl;
        CHECK(joined == "x-y-z", "Join 正确");
        
        // Trim
        CHECK(Trim("  hello  ") == "hello", "Trim 正确");
        CHECK(TrimLeft("  hello") == "hello", "TrimLeft 正确");
        CHECK(TrimRight("hello  ") == "hello", "TrimRight 正确");
        
        // 大小写
        CHECK(ToLower("HELLO") == "hello", "ToLower 正确");
        CHECK(ToUpper("hello") == "HELLO", "ToUpper 正确");
        
        // 替换
        CHECK(Replace("hello", "l", "L") == "heLlo", "Replace 正确");
        CHECK(ReplaceAll("hello", "l", "L") == "heLLo", "ReplaceAll 正确");
        
        // 前缀后缀
        CHECK(StartsWith("hello", "he"), "StartsWith 正确");
        CHECK(!StartsWith("hello", "lo"), "StartsWith 否定正确");
        CHECK(EndsWith("hello.txt", ".txt"), "EndsWith 正确");
        CHECK(!EndsWith("hello.txt", ".md"), "EndsWith 否定正确");
        
        // 判断
        CHECK(IsDigit("12345"), "IsDigit 正确");
        CHECK(!IsDigit("123a5"), "IsDigit 否定正确");
        CHECK(IsEmpty(""), "IsEmpty 正确");
        CHECK(IsBlank("   "), "IsBlank 正确");
        CHECK(!IsBlank("  a  "), "IsBlank 否定正确");
    }

    // ========================================
    // 6. 类型转换测试
    // ========================================
    TEST("类型转换");
    {
        CHECK(ToInt32("123") == 123, "ToInt32 正确");
        CHECK(ToInt32("abc", -1) == -1, "ToInt32 默认值正确");
        CHECK(ToInt64("9999999999") == 9999999999LL, "ToInt64 正确");
        CHECK(std::abs(ToDouble("3.14") - 3.14) < 0.001, "ToDouble 正确");
    }

    // ========================================
    // 结果汇总
    // ========================================
    std::cout << "\n========================================" << std::endl;
    if (failed == 0) {
        std::cout << "\033[32m所有测试通过!\033[0m" << std::endl;
    } else {
        std::cout << "\033[31m" << failed << " 个测试失败\033[0m" << std::endl;
    }
    std::cout << "========================================" << std::endl;

    return failed;
}
