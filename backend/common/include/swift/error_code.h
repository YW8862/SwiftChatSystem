#pragma once

/**
 * @file error_code.h
 * @brief SwiftChatSystem 错误码与结果类型定义
 * 
 * 编码规则：
 *   0        - 成功
 *   1-99     - 通用错误
 *   100-199  - 认证错误 (AuthSvr)
 *   200-299  - 好友错误 (FriendSvr)
 *   300-399  - 消息错误 (ChatSvr)
 *   400-499  - 文件错误 (FileSvr)
 *   500-599  - 群组错误 (ChatSvr)
 *   600-699  - 网关错误 (GateSvr)
 *   700-799  - 路由错误 (ZoneSvr)
 *   800-899  - 存储错误
 *   900-999  - 网络/RPC错误
 */

#include <string>
#include <variant>
#include <unordered_map>

namespace swift {

// ============================================================================
// 错误码枚举
// ============================================================================

enum class ErrorCode {
    // ========== 成功 ==========
    OK = 0,
    
    // ========== 通用错误 1-99 ==========
    UNKNOWN = 1,
    INVALID_PARAM = 2,
    INTERNAL_ERROR = 3,
    NOT_FOUND = 4,
    ALREADY_EXISTS = 5,
    PERMISSION_DENIED = 6,
    RATE_LIMITED = 7,              // 请求过于频繁
    SERVICE_UNAVAILABLE = 8,       // 服务不可用
    TIMEOUT = 9,                   // 操作超时
    CANCELLED = 10,                // 操作被取消
    DATA_CORRUPTED = 11,           // 数据损坏
    UNSUPPORTED = 12,              // 不支持的操作
    
    // ========== 认证错误 100-199 ==========
    AUTH_FAILED = 100,             // 认证失败
    TOKEN_EXPIRED = 101,           // Token 已过期
    TOKEN_INVALID = 102,           // Token 无效
    USER_NOT_FOUND = 103,          // 用户不存在
    PASSWORD_WRONG = 104,          // 密码错误
    USER_ALREADY_EXISTS = 105,     // 用户已存在
    USERNAME_INVALID = 106,        // 用户名格式无效
    PASSWORD_TOO_WEAK = 107,       // 密码强度不足
    ACCOUNT_DISABLED = 108,        // 账户已禁用
    ACCOUNT_LOCKED = 109,          // 账户已锁定（多次密码错误）
    EMAIL_INVALID = 110,           // 邮箱格式无效
    EMAIL_ALREADY_USED = 111,      // 邮箱已被使用
    PHONE_INVALID = 112,           // 手机号格式无效
    PHONE_ALREADY_USED = 113,      // 手机号已被使用
    VERIFY_CODE_WRONG = 114,       // 验证码错误
    VERIFY_CODE_EXPIRED = 115,     // 验证码已过期
    LOGIN_ELSEWHERE = 116,         // 账号在其他设备登录
    SESSION_INVALID = 117,         // 会话无效
    
    // ========== 好友错误 200-299 ==========
    FRIEND_ALREADY = 200,          // 已经是好友
    FRIEND_NOT_FOUND = 201,        // 好友不存在
    BLOCKED = 202,                 // 已被对方拉黑
    REQUEST_PENDING = 203,         // 好友请求待处理
    REQUEST_NOT_FOUND = 204,       // 好友请求不存在
    REQUEST_EXPIRED = 205,         // 好友请求已过期
    SELF_OPERATION = 206,          // 不能对自己操作
    FRIEND_LIMIT_REACHED = 207,    // 好友数量已达上限
    BLOCK_LIMIT_REACHED = 208,     // 黑名单数量已达上限
    ALREADY_BLOCKED = 209,         // 已在黑名单中
    NOT_BLOCKED = 210,             // 不在黑名单中
    FRIEND_GROUP_DEFAULT = 211,    // 默认分组不能删除
    FRIEND_GROUP_NOT_FOUND = 212,  // 好友分组不存在

    // ========== 消息错误 300-399 ==========
    MSG_NOT_FOUND = 300,           // 消息不存在
    RECALL_TIMEOUT = 301,          // 超过撤回时间限制
    RECALL_NOT_ALLOWED = 302,      // 无权撤回此消息
    MSG_TOO_LONG = 303,            // 消息内容过长
    MSG_EMPTY = 304,               // 消息内容为空
    MSG_TYPE_INVALID = 305,        // 消息类型无效
    CONVERSATION_NOT_FOUND = 306,  // 会话不存在
    MSG_SEND_FAILED = 307,         // 消息发送失败
    MSG_ALREADY_READ = 308,        // 消息已读
    OFFLINE_MSG_LIMIT = 309,       // 离线消息数量超限
    MSG_FILTERED = 310,            // 消息被过滤（敏感词）
    RECEIVER_OFFLINE = 311,        // 接收方不在线
    RECEIVER_BLOCKED = 312,        // 被接收方拉黑
    CONVERSATION_PRIVATE_CANNOT_DELETE = 313,  // 私聊不能删除会话，需删除好友后由系统处理
    
    // ========== 文件错误 400-499 ==========
    FILE_TOO_LARGE = 400,          // 文件过大
    FILE_TYPE_NOT_ALLOWED = 401,   // 文件类型不允许
    UPLOAD_FAILED = 402,           // 上传失败
    DOWNLOAD_FAILED = 403,         // 下载失败
    FILE_NOT_FOUND = 404,          // 文件不存在
    FILE_EXPIRED = 405,            // 文件已过期
    STORAGE_FULL = 406,            // 存储空间已满
    FILE_CORRUPTED = 407,          // 文件损坏
    CHECKSUM_MISMATCH = 408,       // 文件校验失败
    UPLOAD_INCOMPLETE = 409,       // 上传未完成
    FILE_LOCKED = 410,             // 文件被锁定
    THUMBNAIL_FAILED = 411,        // 缩略图生成失败
    
    // ========== 群组错误 500-599 ==========
    GROUP_NOT_FOUND = 500,         // 群组不存在
    GROUP_ALREADY_EXISTS = 501,    // 群组已存在
    GROUP_FULL = 502,              // 群组人数已满
    NOT_GROUP_MEMBER = 503,        // 不是群成员
    ALREADY_GROUP_MEMBER = 504,    // 已是群成员
    NOT_GROUP_ADMIN = 505,         // 不是群管理员
    NOT_GROUP_OWNER = 506,         // 不是群主
    GROUP_DISBANDED = 507,         // 群组已解散
    INVITE_NOT_ALLOWED = 508,      // 不允许邀请
    JOIN_NOT_ALLOWED = 509,        // 不允许加入
    KICK_NOT_ALLOWED = 510,        // 不允许踢出
    GROUP_MUTED = 511,             // 群组已禁言
    MEMBER_MUTED = 512,            // 成员已被禁言
    GROUP_NAME_INVALID = 513,      // 群名无效
    ADMIN_LIMIT_REACHED = 514,     // 管理员数量已达上限
    OWNER_CANNOT_LEAVE = 515,      // 群主不能退群
    CANNOT_KICK_ADMIN = 516,       // 不能踢出管理员
    GROUP_MEMBERS_TOO_FEW = 517,   // 建群至少需 3 人（含创建者），不允许 1 人或 2 人建群
    
    // ========== 网关错误 600-699 ==========
    CONNECTION_CLOSED = 600,       // 连接已关闭
    CONNECTION_TIMEOUT = 601,      // 连接超时
    HANDSHAKE_FAILED = 602,        // 握手失败
    PROTOCOL_ERROR = 603,          // 协议错误
    MESSAGE_TOO_LARGE = 604,       // 消息包过大
    INVALID_FRAME = 605,           // 无效的帧格式
    PING_TIMEOUT = 606,            // 心跳超时
    TOO_MANY_CONNECTIONS = 607,    // 连接数过多
    KICK_BY_SERVER = 608,          // 被服务器踢出
    DUPLICATE_LOGIN = 609,         // 重复登录
    
    // ========== 路由错误 700-799 ==========
    ROUTE_NOT_FOUND = 700,         // 路由不存在
    USER_OFFLINE = 701,            // 用户不在线
    GATE_NOT_FOUND = 702,          // 网关不存在
    GATE_UNAVAILABLE = 703,        // 网关不可用
    SESSION_NOT_FOUND = 704,       // 会话不存在
    FORWARD_FAILED = 705,          // 转发失败
    BROADCAST_FAILED = 706,        // 广播失败
    
    // ========== 存储错误 800-899 ==========
    DB_CONNECTION_FAILED = 800,    // 数据库连接失败
    DB_QUERY_FAILED = 801,         // 数据库查询失败
    DB_WRITE_FAILED = 802,         // 数据库写入失败
    DB_TRANSACTION_FAILED = 803,   // 事务失败
    CACHE_MISS = 804,              // 缓存未命中
    CACHE_WRITE_FAILED = 805,      // 缓存写入失败
    REDIS_CONNECTION_FAILED = 806, // Redis 连接失败
    ROCKSDB_ERROR = 807,           // RocksDB 错误
    
    // ========== 网络/RPC错误 900-999 ==========
    RPC_FAILED = 900,              // RPC 调用失败
    RPC_TIMEOUT = 901,             // RPC 超时
    RPC_CANCELLED = 902,           // RPC 被取消
    SERVICE_NOT_FOUND = 903,       // 服务未找到
    NETWORK_ERROR = 904,           // 网络错误
    DNS_FAILED = 905,              // DNS 解析失败
    SSL_ERROR = 906,               // SSL/TLS 错误
};

// ============================================================================
// 错误码转字符串
// ============================================================================

/**
 * 获取错误码描述
 */
inline const char* ErrorCodeToString(ErrorCode code) {
    static const std::unordered_map<ErrorCode, const char*> messages = {
        // ========== 成功 ==========
        {ErrorCode::OK, "success"},
        
        // ========== 通用错误 1-99 ==========
        {ErrorCode::UNKNOWN, "unknown error"},
        {ErrorCode::INVALID_PARAM, "invalid parameter"},
        {ErrorCode::INTERNAL_ERROR, "internal error"},
        {ErrorCode::NOT_FOUND, "not found"},
        {ErrorCode::ALREADY_EXISTS, "already exists"},
        {ErrorCode::PERMISSION_DENIED, "permission denied"},
        {ErrorCode::RATE_LIMITED, "rate limited"},
        {ErrorCode::SERVICE_UNAVAILABLE, "service unavailable"},
        {ErrorCode::TIMEOUT, "timeout"},
        {ErrorCode::CANCELLED, "cancelled"},
        {ErrorCode::DATA_CORRUPTED, "data corrupted"},
        {ErrorCode::UNSUPPORTED, "unsupported"},
        
        // ========== 认证错误 100-199 ==========
        {ErrorCode::AUTH_FAILED, "authentication failed"},
        {ErrorCode::TOKEN_EXPIRED, "token expired"},
        {ErrorCode::TOKEN_INVALID, "token invalid"},
        {ErrorCode::USER_NOT_FOUND, "user not found"},
        {ErrorCode::PASSWORD_WRONG, "password wrong"},
        {ErrorCode::USER_ALREADY_EXISTS, "user already exists"},
        {ErrorCode::USERNAME_INVALID, "username invalid"},
        {ErrorCode::PASSWORD_TOO_WEAK, "password too weak"},
        {ErrorCode::ACCOUNT_DISABLED, "account disabled"},
        {ErrorCode::ACCOUNT_LOCKED, "account locked"},
        {ErrorCode::EMAIL_INVALID, "email invalid"},
        {ErrorCode::EMAIL_ALREADY_USED, "email already used"},
        {ErrorCode::PHONE_INVALID, "phone invalid"},
        {ErrorCode::PHONE_ALREADY_USED, "phone already used"},
        {ErrorCode::VERIFY_CODE_WRONG, "verify code wrong"},
        {ErrorCode::VERIFY_CODE_EXPIRED, "verify code expired"},
        {ErrorCode::LOGIN_ELSEWHERE, "login elsewhere"},
        {ErrorCode::SESSION_INVALID, "session invalid"},
        
        // ========== 好友错误 200-299 ==========
        {ErrorCode::FRIEND_ALREADY, "already friends"},
        {ErrorCode::FRIEND_NOT_FOUND, "friend not found"},
        {ErrorCode::BLOCKED, "blocked by user"},
        {ErrorCode::REQUEST_PENDING, "request pending"},
        {ErrorCode::REQUEST_NOT_FOUND, "request not found"},
        {ErrorCode::REQUEST_EXPIRED, "request expired"},
        {ErrorCode::SELF_OPERATION, "cannot operate on self"},
        {ErrorCode::FRIEND_LIMIT_REACHED, "friend limit reached"},
        {ErrorCode::BLOCK_LIMIT_REACHED, "block limit reached"},
        {ErrorCode::ALREADY_BLOCKED, "already blocked"},
        {ErrorCode::NOT_BLOCKED, "not blocked"},
        
        // ========== 消息错误 300-399 ==========
        {ErrorCode::MSG_NOT_FOUND, "message not found"},
        {ErrorCode::RECALL_TIMEOUT, "recall timeout"},
        {ErrorCode::RECALL_NOT_ALLOWED, "recall not allowed"},
        {ErrorCode::MSG_TOO_LONG, "message too long"},
        {ErrorCode::MSG_EMPTY, "message empty"},
        {ErrorCode::MSG_TYPE_INVALID, "message type invalid"},
        {ErrorCode::CONVERSATION_NOT_FOUND, "conversation not found"},
        {ErrorCode::MSG_SEND_FAILED, "message send failed"},
        {ErrorCode::MSG_ALREADY_READ, "message already read"},
        {ErrorCode::OFFLINE_MSG_LIMIT, "offline message limit"},
        {ErrorCode::MSG_FILTERED, "message filtered"},
        {ErrorCode::RECEIVER_OFFLINE, "receiver offline"},
        {ErrorCode::RECEIVER_BLOCKED, "receiver blocked"},
        {ErrorCode::CONVERSATION_PRIVATE_CANNOT_DELETE, "private conversation cannot be deleted"},
        
        // ========== 文件错误 400-499 ==========
        {ErrorCode::FILE_TOO_LARGE, "file too large"},
        {ErrorCode::FILE_TYPE_NOT_ALLOWED, "file type not allowed"},
        {ErrorCode::UPLOAD_FAILED, "upload failed"},
        {ErrorCode::DOWNLOAD_FAILED, "download failed"},
        {ErrorCode::FILE_NOT_FOUND, "file not found"},
        {ErrorCode::FILE_EXPIRED, "file expired"},
        {ErrorCode::STORAGE_FULL, "storage full"},
        {ErrorCode::FILE_CORRUPTED, "file corrupted"},
        {ErrorCode::CHECKSUM_MISMATCH, "checksum mismatch"},
        {ErrorCode::UPLOAD_INCOMPLETE, "upload incomplete"},
        {ErrorCode::FILE_LOCKED, "file locked"},
        {ErrorCode::THUMBNAIL_FAILED, "thumbnail failed"},
        
        // ========== 群组错误 500-599 ==========
        {ErrorCode::GROUP_NOT_FOUND, "group not found"},
        {ErrorCode::GROUP_ALREADY_EXISTS, "group already exists"},
        {ErrorCode::GROUP_FULL, "group full"},
        {ErrorCode::NOT_GROUP_MEMBER, "not group member"},
        {ErrorCode::ALREADY_GROUP_MEMBER, "already group member"},
        {ErrorCode::NOT_GROUP_ADMIN, "not group admin"},
        {ErrorCode::NOT_GROUP_OWNER, "not group owner"},
        {ErrorCode::GROUP_DISBANDED, "group disbanded"},
        {ErrorCode::INVITE_NOT_ALLOWED, "invite not allowed"},
        {ErrorCode::JOIN_NOT_ALLOWED, "join not allowed"},
        {ErrorCode::KICK_NOT_ALLOWED, "kick not allowed"},
        {ErrorCode::GROUP_MUTED, "group muted"},
        {ErrorCode::MEMBER_MUTED, "member muted"},
        {ErrorCode::GROUP_NAME_INVALID, "group name invalid"},
        {ErrorCode::ADMIN_LIMIT_REACHED, "admin limit reached"},
        {ErrorCode::OWNER_CANNOT_LEAVE, "owner cannot leave"},
        {ErrorCode::CANNOT_KICK_ADMIN, "cannot kick admin"},
        {ErrorCode::GROUP_MEMBERS_TOO_FEW, "group requires at least 3 members"},
        
        // ========== 网关错误 600-699 ==========
        {ErrorCode::CONNECTION_CLOSED, "connection closed"},
        {ErrorCode::CONNECTION_TIMEOUT, "connection timeout"},
        {ErrorCode::HANDSHAKE_FAILED, "handshake failed"},
        {ErrorCode::PROTOCOL_ERROR, "protocol error"},
        {ErrorCode::MESSAGE_TOO_LARGE, "message too large"},
        {ErrorCode::INVALID_FRAME, "invalid frame"},
        {ErrorCode::PING_TIMEOUT, "ping timeout"},
        {ErrorCode::TOO_MANY_CONNECTIONS, "too many connections"},
        {ErrorCode::KICK_BY_SERVER, "kick by server"},
        {ErrorCode::DUPLICATE_LOGIN, "duplicate login"},
        
        // ========== 路由错误 700-799 ==========
        {ErrorCode::ROUTE_NOT_FOUND, "route not found"},
        {ErrorCode::USER_OFFLINE, "user offline"},
        {ErrorCode::GATE_NOT_FOUND, "gate not found"},
        {ErrorCode::GATE_UNAVAILABLE, "gate unavailable"},
        {ErrorCode::SESSION_NOT_FOUND, "session not found"},
        {ErrorCode::FORWARD_FAILED, "forward failed"},
        {ErrorCode::BROADCAST_FAILED, "broadcast failed"},
        
        // ========== 存储错误 800-899 ==========
        {ErrorCode::DB_CONNECTION_FAILED, "database connection failed"},
        {ErrorCode::DB_QUERY_FAILED, "database query failed"},
        {ErrorCode::DB_WRITE_FAILED, "database write failed"},
        {ErrorCode::DB_TRANSACTION_FAILED, "database transaction failed"},
        {ErrorCode::CACHE_MISS, "cache miss"},
        {ErrorCode::CACHE_WRITE_FAILED, "cache write failed"},
        {ErrorCode::REDIS_CONNECTION_FAILED, "redis connection failed"},
        {ErrorCode::ROCKSDB_ERROR, "rocksdb error"},
        
        // ========== 网络/RPC错误 900-999 ==========
        {ErrorCode::RPC_FAILED, "rpc failed"},
        {ErrorCode::RPC_TIMEOUT, "rpc timeout"},
        {ErrorCode::RPC_CANCELLED, "rpc cancelled"},
        {ErrorCode::SERVICE_NOT_FOUND, "service not found"},
        {ErrorCode::NETWORK_ERROR, "network error"},
        {ErrorCode::DNS_FAILED, "dns failed"},
        {ErrorCode::SSL_ERROR, "ssl error"},
    };
    
    auto it = messages.find(code);
    return it != messages.end() ? it->second : "unknown";
}

/**
 * 获取错误码数值
 */
inline int ErrorCodeToInt(ErrorCode code) {
    return static_cast<int>(code);
}

// ============================================================================
// Error 类
// ============================================================================

/**
 * 错误信息
 */
struct Error {
    ErrorCode code;
    std::string message;
    
    Error(ErrorCode c = ErrorCode::OK) 
        : code(c), message(ErrorCodeToString(c)) {}
    
    Error(ErrorCode c, const std::string& msg) 
        : code(c), message(msg) {}
    
    bool ok() const { return code == ErrorCode::OK; }
    
    int Code() const { return ErrorCodeToInt(code); }
    
    // 便于日志输出
    std::string ToString() const {
        return "[" + std::to_string(Code()) + "] " + message;
    }
};

// ============================================================================
// Result<T> 模板类
// ============================================================================

/**
 * 结果类型（携带返回值或错误）
 * 
 * @example
 *   Result<User> GetUser(int64_t id) {
 *       if (id <= 0) return ErrorCode::INVALID_PARAM;
 *       // ...
 *       return user;
 *   }
 *   
 *   auto result = GetUser(123);
 *   if (result) {
 *       LogInfo("User: " << result->name);
 *   } else {
 *       LogError(result.error().ToString());
 *   }
 */
template<typename T>
class Result {
public:
    // 成功构造
    Result(const T& value) : data_(value) {}
    Result(T&& value) : data_(std::move(value)) {}
    
    // 失败构造
    Result(const Error& error) : data_(error) {}
    Result(Error&& error) : data_(std::move(error)) {}
    Result(ErrorCode code) : data_(Error(code)) {}
    Result(ErrorCode code, const std::string& msg) : data_(Error(code, msg)) {}
    
    // 状态判断
    bool ok() const { return std::holds_alternative<T>(data_); }
    operator bool() const { return ok(); }
    
    // 获取值（成功时）
    const T& value() const { return std::get<T>(data_); }
    T& value() { return std::get<T>(data_); }
    
    const T* operator->() const { return &std::get<T>(data_); }
    T* operator->() { return &std::get<T>(data_); }
    
    const T& operator*() const { return value(); }
    T& operator*() { return value(); }
    
    // 获取错误（失败时）
    const Error& error() const { return std::get<Error>(data_); }
    
    // 带默认值的获取
    T value_or(const T& default_value) const {
        return ok() ? value() : default_value;
    }
    
private:
    std::variant<T, Error> data_;
};

// ============================================================================
// Status 类（无返回值）
// ============================================================================

/**
 * 状态类型（仅表示成功/失败，无返回值）
 * 
 * @example
 *   Status DeleteUser(int64_t id) {
 *       if (!exists) return ErrorCode::USER_NOT_FOUND;
 *       // ...
 *       return Status::OK();
 *   }
 */
class Status {
public:
    Status() : error_(ErrorCode::OK) {}
    Status(ErrorCode code) : error_(code) {}
    Status(ErrorCode code, const std::string& msg) : error_(code, msg) {}
    Status(const Error& error) : error_(error) {}
    
    bool ok() const { return error_.ok(); }
    operator bool() const { return ok(); }
    
    const Error& error() const { return error_; }
    ErrorCode code() const { return error_.code; }
    const std::string& message() const { return error_.message; }
    
    static Status OK() { return Status(); }
    
private:
    Error error_;
};

}  // namespace swift
