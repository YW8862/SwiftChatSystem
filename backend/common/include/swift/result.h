#pragma once

#include <string>
#include <variant>
#include <optional>

namespace swift {

/**
 * 错误码定义
 */
enum class ErrorCode {
    OK = 0,
    
    // 通用错误 1-99
    UNKNOWN = 1,
    INVALID_PARAM = 2,
    INTERNAL_ERROR = 3,
    NOT_FOUND = 4,
    ALREADY_EXISTS = 5,
    PERMISSION_DENIED = 6,
    
    // 认证错误 100-199
    AUTH_FAILED = 100,
    TOKEN_EXPIRED = 101,
    TOKEN_INVALID = 102,
    USER_NOT_FOUND = 103,
    PASSWORD_WRONG = 104,
    
    // 好友错误 200-299
    FRIEND_ALREADY = 200,
    FRIEND_NOT_FOUND = 201,
    BLOCKED = 202,
    REQUEST_PENDING = 203,
    
    // 消息错误 300-399
    MSG_NOT_FOUND = 300,
    RECALL_TIMEOUT = 301,
    RECALL_NOT_ALLOWED = 302,
    
    // 文件错误 400-499
    FILE_TOO_LARGE = 400,
    FILE_TYPE_NOT_ALLOWED = 401,
    UPLOAD_FAILED = 402,
};

/**
 * 错误信息
 */
struct Error {
    ErrorCode code;
    std::string message;
    
    Error(ErrorCode c, const std::string& msg = "") : code(c), message(msg) {}
    
    bool ok() const { return code == ErrorCode::OK; }
};

/**
 * 结果类型
 */
template<typename T>
class Result {
public:
    Result(const T& value) : data_(value) {}
    Result(T&& value) : data_(std::move(value)) {}
    Result(const Error& error) : data_(error) {}
    Result(Error&& error) : data_(std::move(error)) {}
    
    bool ok() const { return std::holds_alternative<T>(data_); }
    
    const T& value() const { return std::get<T>(data_); }
    T& value() { return std::get<T>(data_); }
    
    const Error& error() const { return std::get<Error>(data_); }
    
private:
    std::variant<T, Error> data_;
};

}  // namespace swift
