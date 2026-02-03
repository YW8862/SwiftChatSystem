/**
 * @file jwt_helper.cpp
 * @brief 公共 JWT HS256 实现
 */

#include "swift/jwt_helper.h"
#include "swift/utils.h"
#include <nlohmann/json.hpp>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <cstdio>

using json = nlohmann::json;

namespace swift {

namespace {

std::string Base64UrlEncode(const unsigned char* data, size_t len) {
    static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (size_t i = 0; i < len; ++i) {
        val = (val << 8) + data[i];
        valb += 8;
        while (valb >= 0) {
            out.push_back(b64[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(b64[((val << 8) >> (valb + 8)) & 0x3F]);
    for (char& c : out) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!out.empty() && out.back() == '=') out.pop_back();
    return out;
}

std::string Base64UrlEncode(const std::string& s) {
    return Base64UrlEncode(reinterpret_cast<const unsigned char*>(s.data()), s.size());
}

std::string Base64UrlDecode(const std::string& in) {
    std::string s = in;
    for (char& c : s) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    while (s.size() % 4) s += '=';
    static const int T[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };
    std::string out;
    int val = 0, valb = -8;
    for (unsigned char c : s) {
        if (T[c] < 0) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::string HmacSha256(const std::string& key, const std::string& data) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()), data.size(),
         hash, &len);
    std::string hex;
    for (unsigned int i = 0; i < len; ++i) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02x", hash[i]);
        hex += buf;
    }
    return hex;
}

std::string HexDecode(const std::string& hex) {
    std::string out;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        int a = 0, b = 0;
        if (hex[i] >= '0' && hex[i] <= '9') a = hex[i] - '0';
        else if (hex[i] >= 'a' && hex[i] <= 'f') a = hex[i] - 'a' + 10;
        else if (hex[i] >= 'A' && hex[i] <= 'F') a = hex[i] - 'A' + 10;
        if (hex[i+1] >= '0' && hex[i+1] <= '9') b = hex[i+1] - '0';
        else if (hex[i+1] >= 'a' && hex[i+1] <= 'f') b = hex[i+1] - 'a' + 10;
        else if (hex[i+1] >= 'A' && hex[i+1] <= 'F') b = hex[i+1] - 'A' + 10;
        out.push_back(static_cast<char>((a << 4) | b));
    }
    return out;
}

}  // namespace

std::string JwtCreate(const std::string& user_id,
                      const std::string& secret,
                      int expire_hours,
                      const std::string& issuer) {
    int64_t now = utils::GetTimestampMs() / 1000;
    int64_t exp = now + static_cast<int64_t>(expire_hours) * 3600;

    json header = {{"alg", "HS256"}, {"typ", "JWT"}};
    json payload = {
        {"iss", issuer.empty() ? "swift-online" : issuer},
        {"sub", user_id},
        {"iat", now},
        {"exp", exp}
    };

    std::string header_b64 = Base64UrlEncode(header.dump());
    std::string payload_b64 = Base64UrlEncode(payload.dump());
    std::string msg = header_b64 + "." + payload_b64;

    std::string sig_hex = HmacSha256(secret, msg);
    std::string sig_bin = HexDecode(sig_hex);
    std::string sig_b64 = Base64UrlEncode(
        reinterpret_cast<const unsigned char*>(sig_bin.data()), sig_bin.size());

    return msg + "." + sig_b64;
}

JwtPayload JwtVerify(const std::string& token, const std::string& secret) {
    JwtPayload result;
    result.valid = false;

    size_t dot1 = token.find('.');
    size_t dot2 = token.find('.', dot1 + 1);
    if (dot1 == std::string::npos || dot2 == std::string::npos) return result;

    std::string payload_b64 = token.substr(dot1 + 1, dot2 - dot1 - 1);
    std::string sig_b64 = token.substr(dot2 + 1);
    std::string msg = token.substr(0, dot2);
    std::string sig_hex = HmacSha256(secret, msg);
    std::string sig_bin = HexDecode(sig_hex);
    std::string expected_sig_b64 = Base64UrlEncode(
        reinterpret_cast<const unsigned char*>(sig_bin.data()), sig_bin.size());

    if (sig_b64 != expected_sig_b64) return result;

    try {
        std::string payload_json = Base64UrlDecode(payload_b64);
        json p = json::parse(payload_json);

        result.user_id = p.value("sub", "");
        result.issuer = p.value("iss", "");
        result.iat = p.value("iat", 0);
        result.exp = p.value("exp", 0);

        int64_t now = utils::GetTimestampMs() / 1000;
        if (result.exp < now) return result;
        if (result.issuer != "swift-online" && result.issuer != "swift-auth") return result;
        if (result.user_id.empty()) return result;

        result.valid = true;
    } catch (...) {
        result.valid = false;
    }

    return result;
}

}  // namespace swift
