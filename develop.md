# SwiftChatSystem 开发指南

从零开始完整开发一个高性能社交平台的详细步骤。

---

## 目录

1. [开发环境准备](#1-开发环境准备)
2. [项目构建配置](#2-项目构建配置)
3. [阶段一：AsyncLogger 异步日志库](#3-阶段一asynclogger-异步日志库)
4. [阶段二：公共模块与 Proto 定义](#4-阶段二公共模块与-proto-定义)
5. [阶段三：AuthSvr 认证服务](#5-阶段三authsvr-认证服务)
6. [阶段四：FriendSvr 好友服务](#6-阶段四friendsvr-好友服务)
7. [阶段五：ChatSvr 消息服务](#7-阶段五chatsvr-消息服务)
8. [阶段六：FileSvr 文件服务](#8-阶段六filesvr-文件服务)
9. [阶段七：ZoneSvr 路由服务](#9-阶段七zonesvr-路由服务)
10. [阶段八：GateSvr 接入网关](#10-阶段八gatesvr-接入网关)
11. [阶段九：Qt 客户端开发](#11-阶段九qt-客户端开发)
12. [阶段十：容器化与部署](#12-阶段十容器化与部署)
13. [测试与调试](#13-测试与调试)
14. [常见问题](#14-常见问题)

---

## 1. 开发环境准备

### 1.1 系统要求

| 组件 | 版本要求 | 说明 |
|------|---------|------|
| 操作系统 | Ubuntu 20.04+ / Windows 10+ | Linux 推荐用于后端开发 |
| 编译器 | GCC 9+ / Clang 10+ / MSVC 2019+ | 需支持 C++17 |
| CMake | 3.16+ | 构建系统 |
| vcpkg | 最新 | 包管理器 |

### 1.2 安装依赖（Ubuntu）

```bash
# 基础工具
sudo apt update
sudo apt install -y build-essential cmake git curl zip unzip tar pkg-config

# 安装 vcpkg
cd ~
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
echo 'export VCPKG_ROOT=~/vcpkg' >> ~/.bashrc
echo 'export PATH=$VCPKG_ROOT:$PATH' >> ~/.bashrc
source ~/.bashrc

# 安装项目依赖
vcpkg install grpc protobuf rocksdb boost-beast boost-asio openssl nlohmann-json

# 可选：Qt（客户端开发）
sudo apt install -y qt5-default qtwebsockets5-dev

# 可选：Docker + Minikube（部署）
sudo apt install -y docker.io
curl -LO https://storage.googleapis.com/minikube/releases/latest/minikube-linux-amd64
sudo install minikube-linux-amd64 /usr/local/bin/minikube
```

### 1.3 安装依赖（Windows）

```powershell
# 安装 vcpkg
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
# 设置环境变量 VCPKG_ROOT=C:\vcpkg

# 安装依赖
vcpkg install grpc:x64-windows protobuf:x64-windows rocksdb:x64-windows ^
              boost-beast:x64-windows openssl:x64-windows nlohmann-json:x64-windows

# Qt 安装：下载 Qt 在线安装器，选择 Qt 5.15 + MSVC 2019
```

### 1.4 IDE 配置

**VSCode 推荐插件：**
- C/C++ (Microsoft)
- CMake Tools
- Protobuf support
- clangd（代码补全）

**CLion：**
- 设置 CMake 工具链，指定 vcpkg toolchain file

```
Settings → Build → CMake → CMake options:
-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

---

## 2. 项目构建配置

### 2.1 克隆项目

```bash
cd ~/projects
git clone <repo-url> SwiftChatSystem
cd SwiftChatSystem
```

### 2.2 目录结构确认

```bash
tree -L 2
# 应该看到：
# ├── backend/
# ├── client/
# ├── deploy/
# ├── CMakeLists.txt
# ├── vcpkg.json
# └── system.md
```

### 2.3 首次构建测试

```bash
# 创建构建目录
mkdir build && cd build

# 配置（使用 vcpkg）
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
         -DCMAKE_BUILD_TYPE=Debug

# 编译（首次会比较慢，需要编译 Proto）
make -j$(nproc)

# 如果成功，会生成各服务的可执行文件
ls backend/*/
```

### 2.4 常见构建问题

| 问题 | 解决方案 |
|------|---------|
| `protoc not found` | 确认 vcpkg 安装了 protobuf，或手动设置 `Protobuf_PROTOC_EXECUTABLE` |
| `grpc_cpp_plugin not found` | vcpkg 安装 grpc 后会自动安装，检查路径 |
| `rocksdb link error` | 确认安装了 `libsnappy-dev libzstd-dev liblz4-dev` |

---

## 3. 阶段一：AsyncLogger 异步日志库

AsyncLogger 是独立的库，建议先完成它，因为所有服务都依赖日志功能。

### 3.1 目标

- 高性能异步日志
- 无锁环形缓冲区
- 文件滚动
- 支持多 Sink

### 3.2 实现步骤

#### Step 1: 实现 RingBuffer

```cpp
// async_logger/src/ring_buffer.h

#include <atomic>
#include <vector>
#include <cstring>

class RingBuffer {
public:
    explicit RingBuffer(size_t capacity);
    
    // 写入数据（生产者调用）
    bool Write(const char* data, size_t len);
    
    // 读取数据（消费者调用）
    size_t Read(char* buffer, size_t max_len);
    
    // 获取可读数据大小
    size_t ReadableSize() const;
    
    bool IsFull() const;
    bool IsEmpty() const;
    
private:
    std::vector<char> buffer_;
    std::atomic<size_t> write_pos_{0};
    std::atomic<size_t> read_pos_{0};
    size_t capacity_;
};
```

**关键点：**
- 使用 `std::atomic` 保证线程安全
- 环形缓冲区避免内存分配
- 双缓冲交换策略

#### Step 2: 实现 Sink 接口

```cpp
// async_logger/src/sink.h

class Sink {
public:
    virtual ~Sink() = default;
    virtual void Write(const char* data, size_t len) = 0;
    virtual void Flush() = 0;
};

// 控制台输出
class ConsoleSink : public Sink {
public:
    void Write(const char* data, size_t len) override {
        std::cout.write(data, len);
    }
    void Flush() override {
        std::cout.flush();
    }
};

// 文件输出（带滚动）
class FileSink : public Sink {
public:
    FileSink(const std::string& dir, const std::string& prefix, 
             size_t max_size, int max_files);
    void Write(const char* data, size_t len) override;
    void Flush() override;
    
private:
    void RotateIfNeeded();
    void OpenNewFile();
    
    std::string dir_;
    std::string prefix_;
    size_t max_size_;
    int max_files_;
    std::ofstream file_;
    size_t current_size_ = 0;
};
```

#### Step 3: 实现 BackendThread

```cpp
// async_logger/src/backend_thread.h

class BackendThread {
public:
    BackendThread(RingBuffer* buffer, std::vector<Sink*> sinks);
    ~BackendThread();
    
    void Start();
    void Stop();
    
private:
    void ThreadFunc();
    
    RingBuffer* buffer_;
    std::vector<Sink*> sinks_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::condition_variable cv_;
    std::mutex mutex_;
};
```

**关键点：**
- 后台线程定期从 RingBuffer 读取并写入 Sink
- 使用条件变量避免忙等待
- 优雅关闭：确保缓冲区数据全部刷盘

#### Step 4: 实现对外接口

```cpp
// async_logger/include/async_logger/async_logger.h

namespace asynclog {

void Init(const LogConfig& config);
void Shutdown();

// 内部使用
void Log(Level level, const char* file, int line, const char* fmt, ...);

}  // namespace asynclog
```

### 3.3 测试

```cpp
// async_logger/tests/test_async_logger.cpp

#include "async_logger/async_logger.h"
#include <thread>
#include <vector>

void TestBasic() {
    asynclog::LogConfig config;
    config.log_dir = "./test_logs";
    config.enable_console = true;
    asynclog::Init(config);
    
    LOG_INFO("Test message: {}", 123);
    LOG_ERROR("Error: {}", "something wrong");
    
    asynclog::Shutdown();
}

void TestConcurrency() {
    asynclog::Init({});
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 10000; j++) {
                LOG_INFO("Thread {} message {}", i, j);
            }
        });
    }
    
    for (auto& t : threads) t.join();
    asynclog::Shutdown();
}
```

### 3.4 性能基准

```cpp
void BenchmarkThroughput() {
    asynclog::Init({});
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; i++) {
        LOG_INFO("Benchmark message {}", i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "1M logs in " << ms << "ms, " 
              << (1000000.0 / ms * 1000) << " logs/sec" << std::endl;
    
    asynclog::Shutdown();
}
```

**目标：** > 100万条/秒

---

## 4. 阶段二：公共模块与 Proto 定义

### 4.1 公共模块

#### 错误码定义

```cpp
// backend/common/include/swift/result.h

enum class ErrorCode {
    OK = 0,
    
    // 通用错误 1-99
    UNKNOWN = 1,
    INVALID_PARAM = 2,
    INTERNAL_ERROR = 3,
    NOT_FOUND = 4,
    ALREADY_EXISTS = 5,
    
    // 认证错误 100-199
    AUTH_FAILED = 100,
    TOKEN_EXPIRED = 101,
    TOKEN_INVALID = 102,
    USER_NOT_FOUND = 103,
    PASSWORD_WRONG = 104,
    
    // ... 更多错误码
};
```

#### 工具函数

```cpp
// backend/common/src/utils.cpp

std::string GenerateUUID() {
    // 使用 UUID 库或自实现
}

int64_t GetTimestampMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string SHA256(const std::string& input) {
    // 使用 OpenSSL
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, input.c_str(), input.size());
    SHA256_Final(hash, &ctx);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}
```

### 4.2 Proto 定义检查

确认各服务的 proto 文件语法正确：

```bash
# 检查 proto 语法
cd backend
for f in */proto/*.proto common/proto/*.proto; do
    echo "Checking $f..."
    protoc --proto_path=common/proto --proto_path=$(dirname $f) \
           --cpp_out=/tmp $f
done
```

### 4.3 编译 Proto

```bash
cd build
make common_proto  # 先编译公共 proto
make -j$(nproc)    # 编译所有
```

验证生成的文件：

```bash
ls backend/authsvr/proto/*.pb.h
# 应该有 auth.pb.h 和 auth.grpc.pb.h
```

---

## 5. 阶段三：AuthSvr 认证服务

AuthSvr 是最基础的服务，其他服务都依赖它进行用户验证。

### 5.1 功能清单

- [x] 用户注册
- [x] 用户登录
- [x] Token 验证
- [x] 获取/更新用户资料
- [x] 密码加密存储
- [x] JWT Token 生成

### 5.2 实现步骤

#### Step 1: 实现 UserStore（RocksDB 版本）

```cpp
// backend/authsvr/internal/store/user_store.cpp

#include "user_store.h"
#include <rocksdb/db.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct RocksDBUserStore::Impl {
    rocksdb::DB* db = nullptr;
};

RocksDBUserStore::RocksDBUserStore(const std::string& db_path) 
    : impl_(std::make_unique<Impl>()) {
    
    rocksdb::Options options;
    options.create_if_missing = true;
    
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &impl_->db);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
    }
}

RocksDBUserStore::~RocksDBUserStore() {
    delete impl_->db;
}

bool RocksDBUserStore::Create(const UserData& user) {
    // 检查用户名是否已存在
    if (UsernameExists(user.username)) {
        return false;
    }
    
    // 序列化用户数据
    json j;
    j["user_id"] = user.user_id;
    j["username"] = user.username;
    j["password_hash"] = user.password_hash;
    j["nickname"] = user.nickname;
    j["avatar_url"] = user.avatar_url;
    j["created_at"] = user.created_at;
    
    std::string value = j.dump();
    
    // 写入 user:{user_id}
    rocksdb::WriteBatch batch;
    batch.Put("user:" + user.user_id, value);
    batch.Put("username:" + user.username, user.user_id);
    
    return impl_->db->Write(rocksdb::WriteOptions(), &batch).ok();
}

std::optional<UserData> RocksDBUserStore::GetById(const std::string& user_id) {
    std::string value;
    auto status = impl_->db->Get(rocksdb::ReadOptions(), "user:" + user_id, &value);
    if (!status.ok()) {
        return std::nullopt;
    }
    
    json j = json::parse(value);
    UserData user;
    user.user_id = j["user_id"];
    user.username = j["username"];
    user.password_hash = j["password_hash"];
    user.nickname = j["nickname"];
    user.avatar_url = j.value("avatar_url", "");
    user.created_at = j["created_at"];
    
    return user;
}

std::optional<UserData> RocksDBUserStore::GetByUsername(const std::string& username) {
    std::string user_id;
    auto status = impl_->db->Get(rocksdb::ReadOptions(), "username:" + username, &user_id);
    if (!status.ok()) {
        return std::nullopt;
    }
    return GetById(user_id);
}

bool RocksDBUserStore::UsernameExists(const std::string& username) {
    std::string value;
    return impl_->db->Get(rocksdb::ReadOptions(), "username:" + username, &value).ok();
}
```

#### Step 2: 实现 AuthService

```cpp
// backend/authsvr/internal/service/auth_service.cpp

#include "auth_service.h"
#include <jwt-cpp/jwt.h>
#include <openssl/sha.h>

AuthService::AuthService(std::shared_ptr<UserStore> store)
    : store_(std::move(store)) {}

AuthService::RegisterResult AuthService::Register(
    const std::string& username,
    const std::string& password,
    const std::string& nickname,
    const std::string& email) {
    
    RegisterResult result;
    
    // 检查用户名是否已存在
    if (store_->UsernameExists(username)) {
        result.success = false;
        result.error = "Username already exists";
        return result;
    }
    
    // 生成用户 ID
    std::string user_id = GenerateUserId();
    
    // 加密密码
    std::string password_hash = HashPassword(password);
    
    // 创建用户
    UserData user;
    user.user_id = user_id;
    user.username = username;
    user.password_hash = password_hash;
    user.nickname = nickname.empty() ? username : nickname;
    user.created_at = swift::utils::GetTimestampMs();
    
    if (store_->Create(user)) {
        result.success = true;
        result.user_id = user_id;
    } else {
        result.success = false;
        result.error = "Failed to create user";
    }
    
    return result;
}

AuthService::LoginResult AuthService::Login(
    const std::string& username,
    const std::string& password,
    const std::string& device_id,
    const std::string& device_type) {
    
    LoginResult result;
    
    // 查找用户
    auto user = store_->GetByUsername(username);
    if (!user) {
        result.success = false;
        result.error = "User not found";
        return result;
    }
    
    // 验证密码
    if (!VerifyPassword(password, user->password_hash)) {
        result.success = false;
        result.error = "Wrong password";
        return result;
    }
    
    // 生成 Token
    result.success = true;
    result.user_id = user->user_id;
    result.token = GenerateToken(user->user_id);
    result.expire_at = swift::utils::GetTimestampMs() + 7 * 24 * 3600 * 1000;  // 7天
    
    return result;
}

std::string AuthService::GenerateToken(const std::string& user_id) {
    auto token = jwt::create()
        .set_issuer("swift-auth")
        .set_subject(user_id)
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24 * 7))
        .sign(jwt::algorithm::hs256{jwt_secret_});
    
    return token;
}

AuthService::TokenResult AuthService::ValidateToken(const std::string& token) {
    TokenResult result;
    
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{jwt_secret_})
            .with_issuer("swift-auth");
        
        verifier.verify(decoded);
        
        result.valid = true;
        result.user_id = decoded.get_subject();
    } catch (const std::exception& e) {
        result.valid = false;
    }
    
    return result;
}

std::string AuthService::HashPassword(const std::string& password) {
    // 使用 SHA256 + salt（生产环境建议使用 bcrypt）
    std::string salted = password + "swift_salt_2026";
    return swift::utils::SHA256(salted);
}

bool AuthService::VerifyPassword(const std::string& password, const std::string& hash) {
    return HashPassword(password) == hash;
}

std::string AuthService::GenerateUserId() {
    return "u_" + swift::utils::GenerateUUID().substr(0, 12);
}
```

#### Step 3: 实现 gRPC Handler

```cpp
// backend/authsvr/internal/handler/auth_handler.cpp

#include "auth_handler.h"
#include "../service/auth_service.h"
#include "auth.grpc.pb.h"

class AuthServiceImpl final : public swift::auth::AuthService::Service {
public:
    explicit AuthServiceImpl(std::shared_ptr<::swift::auth::AuthService> service)
        : service_(std::move(service)) {}
    
    grpc::Status Register(grpc::ServerContext* context,
                          const swift::auth::RegisterRequest* request,
                          swift::auth::RegisterResponse* response) override {
        
        auto result = service_->Register(
            request->username(),
            request->password(),
            request->nickname(),
            request->email()
        );
        
        if (result.success) {
            response->set_code(0);
            response->set_user_id(result.user_id);
        } else {
            response->set_code(1);
            response->set_message(result.error);
        }
        
        return grpc::Status::OK;
    }
    
    grpc::Status Login(grpc::ServerContext* context,
                       const swift::auth::LoginRequest* request,
                       swift::auth::LoginResponse* response) override {
        
        auto result = service_->Login(
            request->username(),
            request->password(),
            request->device_id(),
            request->device_type()
        );
        
        if (result.success) {
            response->set_code(0);
            response->set_user_id(result.user_id);
            response->set_token(result.token);
            response->set_expire_at(result.expire_at);
        } else {
            response->set_code(1);
            response->set_message(result.error);
        }
        
        return grpc::Status::OK;
    }
    
    grpc::Status ValidateToken(grpc::ServerContext* context,
                                const swift::auth::TokenRequest* request,
                                swift::auth::TokenResponse* response) override {
        
        auto result = service_->ValidateToken(request->token());
        
        response->set_code(0);
        response->set_valid(result.valid);
        if (result.valid) {
            response->set_user_id(result.user_id);
        }
        
        return grpc::Status::OK;
    }
    
private:
    std::shared_ptr<::swift::auth::AuthService> service_;
};
```

#### Step 4: 实现 main.cpp

```cpp
// backend/authsvr/cmd/main.cpp

#include <grpcpp/grpcpp.h>
#include <iostream>
#include "../internal/handler/auth_handler.h"
#include "../internal/service/auth_service.h"
#include "../internal/store/user_store.h"
#include "../internal/config/config.h"

int main(int argc, char* argv[]) {
    // 加载配置
    auto config = swift::auth::LoadConfig("config.yaml");
    
    // 初始化日志
    asynclog::LogConfig log_config;
    log_config.log_dir = config.log_dir;
    asynclog::Init(log_config);
    
    LOG_INFO("AuthSvr starting...");
    
    // 初始化存储
    auto store = std::make_shared<swift::auth::RocksDBUserStore>(config.rocksdb_path);
    
    // 初始化服务
    auto service = std::make_shared<swift::auth::AuthService>(store);
    
    // 启动 gRPC 服务器
    std::string server_address = config.host + ":" + std::to_string(config.port);
    
    AuthServiceImpl grpc_service(service);
    
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&grpc_service);
    
    auto server = builder.BuildAndStart();
    LOG_INFO("AuthSvr listening on {}", server_address);
    
    server->Wait();
    
    asynclog::Shutdown();
    return 0;
}
```

### 5.3 测试 AuthSvr

#### 单元测试

```cpp
// backend/authsvr/tests/test_auth_service.cpp

#include <gtest/gtest.h>
#include "../internal/service/auth_service.h"
#include "../internal/store/user_store.h"

class AuthServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = std::make_shared<swift::auth::RocksDBUserStore>("/tmp/test_auth_db");
        service_ = std::make_shared<swift::auth::AuthService>(store_);
    }
    
    void TearDown() override {
        // 清理测试数据库
        std::filesystem::remove_all("/tmp/test_auth_db");
    }
    
    std::shared_ptr<swift::auth::UserStore> store_;
    std::shared_ptr<swift::auth::AuthService> service_;
};

TEST_F(AuthServiceTest, RegisterSuccess) {
    auto result = service_->Register("testuser", "password123", "Test User", "test@example.com");
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.user_id.empty());
}

TEST_F(AuthServiceTest, RegisterDuplicateUsername) {
    service_->Register("testuser", "password123", "Test User", "test@example.com");
    auto result = service_->Register("testuser", "password456", "Test User 2", "test2@example.com");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Username already exists");
}

TEST_F(AuthServiceTest, LoginSuccess) {
    service_->Register("testuser", "password123", "Test User", "test@example.com");
    auto result = service_->Login("testuser", "password123", "device1", "windows");
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.token.empty());
}

TEST_F(AuthServiceTest, LoginWrongPassword) {
    service_->Register("testuser", "password123", "Test User", "test@example.com");
    auto result = service_->Login("testuser", "wrongpassword", "device1", "windows");
    EXPECT_FALSE(result.success);
}

TEST_F(AuthServiceTest, ValidateToken) {
    service_->Register("testuser", "password123", "Test User", "test@example.com");
    auto login_result = service_->Login("testuser", "password123", "device1", "windows");
    
    auto token_result = service_->ValidateToken(login_result.token);
    EXPECT_TRUE(token_result.valid);
    EXPECT_EQ(token_result.user_id, login_result.user_id);
}
```

#### 使用 grpcurl 测试

```bash
# 启动服务
./build/backend/authsvr/authsvr

# 另一个终端，使用 grpcurl 测试
# 注册
grpcurl -plaintext -d '{"username":"alice","password":"123456","nickname":"Alice"}' \
    localhost:9094 swift.auth.AuthService/Register

# 登录
grpcurl -plaintext -d '{"username":"alice","password":"123456"}' \
    localhost:9094 swift.auth.AuthService/Login

# 验证 Token（使用登录返回的 token）
grpcurl -plaintext -d '{"token":"<token>"}' \
    localhost:9094 swift.auth.AuthService/ValidateToken
```

---

## 6. 阶段四：FriendSvr 好友服务

### 6.1 功能清单

- [x] 发送好友请求
- [x] 处理好友请求（接受/拒绝）
- [x] 获取好友列表
- [x] 删除好友
- [x] 好友分组管理
- [x] 黑名单管理

### 6.2 实现步骤

与 AuthSvr 类似：

1. **FriendStore** - 实现 RocksDB 存储
2. **FriendService** - 实现业务逻辑
3. **FriendHandler** - 实现 gRPC 接口
4. **main.cpp** - 服务启动

### 6.3 关键数据结构

```cpp
// RocksDB Key 设计
// friend:{user_id}:{friend_id}           -> FriendData JSON
// friend_req:{request_id}                -> FriendRequestData JSON
// friend_req_to:{to_user_id}:{req_id}    -> request_id（索引）
// friend_group:{user_id}:{group_id}      -> FriendGroupData JSON
// block:{user_id}:{target_id}            -> "1"
```

### 6.4 测试

```bash
# 添加好友（发送请求）
grpcurl -plaintext -d '{"user_id":"u_alice","friend_id":"u_bob","remark":"Hi!"}' \
    localhost:9096 swift.friend.FriendService/AddFriend

# 处理好友请求
grpcurl -plaintext -d '{"user_id":"u_bob","request_id":"req_xxx","accept":true}' \
    localhost:9096 swift.friend.FriendService/HandleFriendRequest

# 获取好友列表
grpcurl -plaintext -d '{"user_id":"u_alice"}' \
    localhost:9096 swift.friend.FriendService/GetFriends
```

---

## 7. 阶段五：ChatSvr 消息服务

ChatSvr 是核心服务，负责消息的存储、查询、撤回等功能。

### 7.1 功能清单

- [x] 发送消息
- [x] 消息撤回（2分钟内）
- [x] 获取历史消息
- [x] 离线消息队列
- [x] 消息搜索
- [x] 已读回执
- [x] 会话管理

### 7.2 关键实现

#### MessageStore 核心方法

```cpp
bool RocksDBMessageStore::Save(const MessageData& msg) {
    json j;
    j["msg_id"] = msg.msg_id;
    j["from_user_id"] = msg.from_user_id;
    j["to_id"] = msg.to_id;
    j["chat_type"] = msg.chat_type;
    j["content"] = msg.content;
    j["timestamp"] = msg.timestamp;
    j["status"] = msg.status;
    
    rocksdb::WriteBatch batch;
    
    // 1. 存储消息本体
    batch.Put("msg:" + msg.msg_id, j.dump());
    
    // 2. 添加到会话时间线索引
    std::string chat_id = BuildChatId(msg.from_user_id, msg.to_id, msg.chat_type);
    std::string timeline_key = fmt::format("chat:{}:{}:{}", 
        chat_id, msg.timestamp, msg.msg_id);
    batch.Put(timeline_key, msg.msg_id);
    
    return db_->Write(rocksdb::WriteOptions(), &batch).ok();
}

std::vector<MessageData> RocksDBMessageStore::GetHistory(
    const std::string& chat_id,
    int chat_type,
    const std::string& before_msg_id,
    int limit) {
    
    std::vector<MessageData> result;
    
    // 使用前缀扫描 + 反向迭代
    std::string prefix = "chat:" + chat_id + ":";
    
    rocksdb::ReadOptions options;
    auto iter = db_->NewIterator(options);
    
    // 定位到 before_msg_id 或末尾
    if (before_msg_id.empty()) {
        // 从最新开始
        iter->SeekForPrev(prefix + "9999999999999999");
    } else {
        // 从指定消息之前开始
        auto before_msg = GetById(before_msg_id);
        if (before_msg) {
            std::string seek_key = fmt::format("chat:{}:{}:{}", 
                chat_id, before_msg->timestamp, before_msg_id);
            iter->SeekForPrev(seek_key);
            iter->Prev();  // 跳过 before_msg_id 本身
        }
    }
    
    // 反向遍历获取消息
    while (iter->Valid() && result.size() < limit) {
        std::string key = iter->key().ToString();
        if (!key.starts_with(prefix)) break;
        
        std::string msg_id = iter->value().ToString();
        auto msg = GetById(msg_id);
        if (msg) {
            result.push_back(*msg);
        }
        
        iter->Prev();
    }
    
    delete iter;
    return result;
}
```

#### 消息撤回

```cpp
ChatService::RecallResult ChatService::RecallMessage(
    const std::string& msg_id, 
    const std::string& user_id) {
    
    RecallResult result;
    
    // 获取消息
    auto msg = msg_store_->GetById(msg_id);
    if (!msg) {
        result.success = false;
        result.error = "Message not found";
        return result;
    }
    
    // 检查是否是发送者
    if (msg->from_user_id != user_id) {
        result.success = false;
        result.error = "Not allowed to recall";
        return result;
    }
    
    // 检查时间（2分钟内）
    int64_t now = swift::utils::GetTimestampMs();
    if (now - msg->timestamp > RECALL_TIMEOUT_SECONDS * 1000) {
        result.success = false;
        result.error = "Recall timeout";
        return result;
    }
    
    // 标记为已撤回
    if (msg_store_->MarkRecalled(msg_id, now)) {
        result.success = true;
    } else {
        result.success = false;
        result.error = "Failed to recall";
    }
    
    return result;
}
```

### 7.3 测试

```bash
# 发送消息
grpcurl -plaintext -d '{
    "from_user_id":"u_alice",
    "to_id":"u_bob",
    "chat_type":1,
    "content":"Hello Bob!",
    "media_type":"text"
}' localhost:9098 swift.chat.ChatService/SendMessage

# 获取历史消息
grpcurl -plaintext -d '{
    "user_id":"u_alice",
    "chat_id":"c_xxx",
    "chat_type":1,
    "limit":20
}' localhost:9098 swift.chat.ChatService/GetHistory

# 撤回消息
grpcurl -plaintext -d '{
    "msg_id":"m_xxx",
    "user_id":"u_alice"
}' localhost:9098 swift.chat.ChatService/RecallMessage
```

---

## 8. 阶段六：FileSvr 文件服务

### 8.1 功能清单

- [x] 文件上传（gRPC 流式）
- [x] 文件下载（HTTP）
- [x] 文件元信息管理
- [x] MD5 秒传检测

### 8.2 关键实现

#### gRPC 流式上传

```cpp
grpc::Status FileServiceImpl::UploadFile(
    grpc::ServerContext* context,
    grpc::ServerReader<swift::file::UploadChunk>* reader,
    swift::file::UploadResponse* response) {
    
    swift::file::UploadChunk chunk;
    std::string file_id;
    std::string file_name;
    std::string content_type;
    std::vector<char> data;
    
    while (reader->Read(&chunk)) {
        if (chunk.has_meta()) {
            // 第一个 chunk 包含元信息
            file_name = chunk.meta().file_name();
            content_type = chunk.meta().content_type();
            file_id = service_->GenerateFileId();
        } else {
            // 后续 chunk 包含文件数据
            const auto& bytes = chunk.chunk();
            data.insert(data.end(), bytes.begin(), bytes.end());
        }
    }
    
    // 保存文件
    auto result = service_->Upload(
        context->peer(),  // uploader
        file_name,
        content_type,
        data
    );
    
    if (result.success) {
        response->set_code(0);
        response->set_file_id(result.file_id);
        response->set_file_url(result.file_url);
    } else {
        response->set_code(1);
        response->set_message(result.error);
    }
    
    return grpc::Status::OK;
}
```

#### HTTP 下载服务

```cpp
// 使用 Boost.Beast 实现简单 HTTP 服务器
void HttpServer::HandleRequest(
    http::request<http::string_body>& req,
    http::response<http::file_body>& res) {
    
    // 解析 URL: /files/{file_id}
    std::string target = req.target();
    if (!target.starts_with("/files/")) {
        res.result(http::status::not_found);
        return;
    }
    
    std::string file_id = target.substr(7);
    
    // 获取文件信息
    auto meta = file_store_->GetById(file_id);
    if (!meta) {
        res.result(http::status::not_found);
        return;
    }
    
    // 设置响应头
    res.set(http::field::content_type, meta->content_type);
    res.set(http::field::content_disposition, 
            "attachment; filename=\"" + meta->file_name + "\"");
    
    // 返回文件内容
    res.body().open(meta->storage_path.c_str(), beast::file_mode::scan);
    res.prepare_payload();
}
```

---

## 9. 阶段七：ZoneSvr 路由服务

ZoneSvr 是系统的 **API Gateway**，统一持有所有后端服务的 RPC Client，负责请求转发和消息路由。
实际的业务逻辑在各个后端服务（AuthSvr, ChatSvr 等）中实现。

### 9.1 API Gateway 架构

```
┌────────────────────────────────────────────────────────────────────┐
│                      ZoneSvr (API Gateway)                         │
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                    RPC Clients (统一持有)                     │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐ │  │
│  │  │AuthClient  │ │ChatClient  │ │FriendClient│ │FileClient  │ │  │
│  │  └─────┬──────┘ └─────┬──────┘ └─────┬──────┘ └─────┬──────┘ │  │
│  └────────┼──────────────┼──────────────┼──────────────┼────────┘  │
│           │              │              │              │           │
│  ┌────────┼──────────────┼──────────────┼──────────────┼────────┐  │
│  │        │         SessionStore (消息路由)            │        │  │
│  └────────┼──────────────┼──────────────┼──────────────┼────────┘  │
└───────────┼──────────────┼──────────────┼──────────────┼───────────┘
            │              │              │              │
            ▼              ▼              ▼              ▼
       ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐
       │ AuthSvr │    │ ChatSvr │───→│ FileSvr │    │FriendSvr│
       │(实际逻辑)│   │(实际逻辑)│    │(实际逻辑)│   │(实际逻辑)│
       └─────────┘    └─────────┘    └─────────┘    └─────────┘
                           │              ↑
                           └──────────────┘
                          (服务间直接调用)
```

**职责划分：**

| 组件 | 职责 |
|------|------|
| **ZoneSvr** | 统一入口、持有 RPC Client、请求转发、消息路由 |
| **各 System** | 封装对应服务的 RPC 调用接口（转发层） |
| **SessionStore** | 用户会话状态，用于消息路由 |
| **后端 Svr** | **实际业务逻辑**、数据存储 |
| **服务间调用** | 后端服务自己持有对方 Client（如 ChatSvr → FileSvr） |

### 9.2 目录结构

```
backend/zonesvr/
├── cmd/main.cpp
└── internal/
    ├── system/                    # RPC 转发层（封装各服务调用）
    │   ├── base_system.h          # System 基类
    │   ├── system_manager.h/cpp   # System 管理器
    │   ├── auth_system.h/cpp      # AuthSvr 转发层
    │   ├── chat_system.h/cpp      # ChatSvr 转发层
    │   ├── friend_system.h/cpp    # FriendSvr 转发层
    │   ├── group_system.h/cpp     # GroupService 转发层
    │   └── file_system.h/cpp      # FileSvr 转发层
    ├── rpc/                       # RPC 客户端
    │   ├── rpc_client_base.h/cpp  # RPC 客户端基类
    │   ├── auth_rpc_client.h/cpp
    │   ├── chat_rpc_client.h/cpp
    │   ├── friend_rpc_client.h/cpp
    │   ├── group_rpc_client.h/cpp
    │   ├── file_rpc_client.h/cpp
    │   └── gate_rpc_client.h/cpp  # 推送消息到 Gate
    ├── handler/                   # gRPC 入口
    ├── store/                     # SessionStore（会话管理）
    └── config/
```

### 9.3 功能清单

- [x] 用户上线/下线管理
- [x] Gate 节点注册与心跳
- [x] 消息路由（查 SessionStore，推送到 Gate）
- [x] 广播消息
- [x] 在线状态查询
- [x] API Gateway 架构框架

### 9.4 System 基类设计

```cpp
// internal/system/base_system.h
// System 是 RPC 转发层，实际业务逻辑在后端服务

class BaseSystem {
public:
    virtual ~BaseSystem() = default;
    
    /// 系统名称
    virtual std::string Name() const = 0;
    
    /// 初始化（建立 RPC 连接）
    virtual bool Init() = 0;
    
    /// 关闭（清理资源）
    virtual void Shutdown() = 0;

protected:
    /// 共享会话存储，用于消息路由
    std::shared_ptr<SessionStore> session_store_;
};
```

### 9.5 SystemManager 使用示例

```cpp
// 初始化
SystemManager manager;
manager.Init(config);

// 获取 System，转发请求到后端服务
auto* auth = manager.GetAuthSystem();
auto response = auth->ValidateToken(request);  // → AuthSvr RPC

auto* chat = manager.GetChatSystem();
auto result = chat->SendMessage(request);      // → ChatSvr RPC

// 消息路由（ZoneSvr 特有职责）
chat->PushToUser(user_id, "chat.new_message", payload);  // → GateSvr RPC

// 关闭
manager.Shutdown();
```

### 9.6 请求处理流程

```
1. Client → GateSvr (WebSocket)
2. GateSvr → ZoneSvr.HandleRequest(cmd, payload)
3. ZoneSvr Handler 根据 cmd 选择对应 System：
   - "auth.*"   → AuthSystem → AuthSvr (RPC)
   - "chat.*"   → ChatSystem → ChatSvr (RPC)
   - "friend.*" → FriendSystem → FriendSvr (RPC)
   - "group.*"  → GroupSystem → ChatSvr/GroupService (RPC)
   - "file.*"   → FileSystem → FileSvr (RPC)
4. 后端服务处理业务逻辑，返回结果
5. ZoneSvr 返回给 GateSvr → Client

注意：实际业务逻辑在后端服务实现，System 只做 RPC 转发
```

### 9.7 消息路由流程

```
发送消息时的路由流程：

1. Client A → GateSvr-1 (发送消息)
2. GateSvr-1 → ZoneSvr.ChatSystem.SendMessage()
3. ChatSystem → ChatSvr.SendMessage() 存储消息
4. ChatSvr 返回消息ID
5. ChatSystem 查询 SessionStore：用户 B 在哪个 Gate？
6. SessionStore 返回：用户 B 在 GateSvr-2
7. ChatSystem → GateSvr-2.PushMessage() 推送给用户 B
8. GateSvr-2 → Client B (WebSocket 推送)
```

### 9.8 关键实现

#### SessionStore（Redis 版本）

```cpp
// 使用 hiredis 或 redis-plus-plus

bool RedisSessionStore::SetOnline(const UserSession& session) {
    // HSET session:{user_id} gate_id {gate_id} gate_addr {addr} ...
    auto reply = redis_->command(
        "HSET session:%s gate_id %s gate_addr %s device_type %s online_at %lld",
        session.user_id.c_str(),
        session.gate_id.c_str(),
        session.gate_addr.c_str(),
        session.device_type.c_str(),
        session.online_at
    );
    
    // 设置过期时间
    redis_->command("EXPIRE session:%s 3600", session.user_id.c_str());
    
    return reply != nullptr;
}

std::optional<UserSession> RedisSessionStore::GetSession(const std::string& user_id) {
    auto reply = redis_->command("HGETALL session:%s", user_id.c_str());
    if (!reply || reply->type != REDIS_REPLY_ARRAY || reply->elements == 0) {
        return std::nullopt;
    }
    
    UserSession session;
    session.user_id = user_id;
    
    for (size_t i = 0; i < reply->elements; i += 2) {
        std::string field = reply->element[i]->str;
        std::string value = reply->element[i + 1]->str;
        
        if (field == "gate_id") session.gate_id = value;
        else if (field == "gate_addr") session.gate_addr = value;
        else if (field == "device_type") session.device_type = value;
        else if (field == "online_at") session.online_at = std::stoll(value);
    }
    
    return session;
}
```

#### 消息路由

```cpp
ZoneService::RouteResult ZoneService::RouteToUser(
    const std::string& user_id,
    const std::string& cmd,
    const std::string& payload) {
    
    RouteResult result;
    
    // 查询用户会话
    auto session = store_->GetSession(user_id);
    if (!session) {
        result.delivered = false;
        result.user_online = false;
        return result;
    }
    
    result.user_online = true;
    result.gate_id = session->gate_id;
    
    // 调用 Gate 的 gRPC 接口推送消息
    result.delivered = PushToGate(session->gate_addr, user_id, cmd, payload);
    
    return result;
}

bool ZoneService::PushToGate(
    const std::string& gate_addr,
    const std::string& user_id,
    const std::string& cmd,
    const std::string& payload) {
    
    // 创建 gRPC channel（可以使用连接池优化）
    auto channel = grpc::CreateChannel(gate_addr, grpc::InsecureChannelCredentials());
    auto stub = swift::gate::GateInternalService::NewStub(channel);
    
    swift::gate::PushMessageRequest request;
    request.set_user_id(user_id);
    request.set_cmd(cmd);
    request.set_payload(payload);
    
    swift::common::CommonResponse response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    
    auto status = stub->PushMessage(&context, request, &response);
    
    return status.ok() && response.code() == 0;
}
```

---

## 10. 阶段八：GateSvr 接入网关

GateSvr 是客户端的接入点，负责 WebSocket 连接管理和协议转换。

### 10.1 功能清单

- [x] WebSocket 服务器
- [x] 连接管理
- [x] Protobuf 协议解析
- [x] 心跳检测
- [x] 消息转发

### 10.2 关键实现

#### WebSocket 服务器（Boost.Beast）

```cpp
// 使用 Boost.Beast 实现 WebSocket 服务器

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    explicit WebSocketSession(tcp::socket&& socket, GateService* service)
        : ws_(std::move(socket)), service_(service) {
        conn_id_ = GenerateConnId();
    }
    
    void Run() {
        // 接受 WebSocket 握手
        ws_.async_accept([self = shared_from_this()](beast::error_code ec) {
            if (!ec) {
                self->service_->OnConnect(self->conn_id_);
                self->DoRead();
            }
        });
    }
    
    void Send(const std::string& data) {
        ws_.async_write(
            net::buffer(data),
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (ec) {
                    self->Close();
                }
            }
        );
    }
    
private:
    void DoRead() {
        ws_.async_read(buffer_, [self = shared_from_this()](beast::error_code ec, std::size_t) {
            if (ec) {
                self->Close();
                return;
            }
            
            // 处理消息
            std::string data = beast::buffers_to_string(self->buffer_.data());
            self->buffer_.consume(self->buffer_.size());
            
            self->service_->OnMessage(self->conn_id_, data);
            
            // 继续读取
            self->DoRead();
        });
    }
    
    void Close() {
        service_->OnDisconnect(conn_id_);
        ws_.close(websocket::close_code::normal);
    }
    
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    GateService* service_;
    std::string conn_id_;
};
```

#### 协议处理

```cpp
void GateService::HandleClientMessage(
    const std::string& conn_id,
    const std::string& data) {
    
    // 解析 Protobuf
    swift::gate::ClientMessage msg;
    if (!msg.ParseFromString(data)) {
        LOG_ERROR("Failed to parse client message");
        return;
    }
    
    std::string cmd = msg.cmd();
    std::string payload = msg.payload();
    std::string request_id = msg.request_id();
    
    // 根据命令类型处理
    if (cmd == "auth.login") {
        HandleLogin(conn_id, payload, request_id);
    } else if (cmd == "heartbeat") {
        HandleHeartbeat(conn_id, request_id);
    } else if (cmd.starts_with("chat.")) {
        // 转发到 ZoneSvr
        ForwardToZone(conn_id, cmd, payload, request_id);
    } else if (cmd.starts_with("friend.")) {
        ForwardToZone(conn_id, cmd, payload, request_id);
    } else {
        LOG_WARN("Unknown command: {}", cmd);
    }
}

void GateService::HandleLogin(
    const std::string& conn_id,
    const std::string& payload,
    const std::string& request_id) {
    
    swift::gate::ClientLoginRequest req;
    req.ParseFromString(payload);
    
    // 验证 Token
    // ... 调用 AuthSvr 验证 ...
    
    // 绑定用户
    BindUser(conn_id, user_id);
    
    // 通知 ZoneSvr 用户上线
    zone_client_->UserOnline(user_id, gate_id_, device_type, device_id);
    
    // 返回成功响应
    SendResponse(conn_id, "auth.login", request_id, 0, "");
}
```

---

## 11. 阶段九：Qt 客户端开发

### 11.1 项目结构

```
client/desktop/
├── src/
│   ├── main.cpp              # 入口
│   ├── ui/                   # UI 组件
│   │   ├── loginwindow.*     # 登录窗口
│   │   ├── mainwindow.*      # 主窗口
│   │   ├── chatwidget.*      # 聊天组件
│   │   └── contactwidget.*   # 联系人组件
│   ├── network/              # 网络层
│   │   ├── websocket_client.*
│   │   └── protocol_handler.*
│   ├── models/               # 数据模型
│   │   ├── user.*
│   │   ├── message.*
│   │   └── conversation.*
│   └── utils/                # 工具类
└── resources/                # 资源文件
```

### 11.2 开发步骤

#### Step 1: WebSocket 连接

```cpp
// network/websocket_client.cpp

void WebSocketClient::connect(const QString& url) {
    m_socket->open(QUrl(url));
}

void WebSocketClient::onConnected() {
    LOG_INFO("WebSocket connected");
    emit connected();
}

void WebSocketClient::onBinaryMessageReceived(const QByteArray& message) {
    emit messageReceived(message);
}

void WebSocketClient::sendMessage(const QByteArray& data) {
    m_socket->sendBinaryMessage(data);
}
```

#### Step 2: 协议处理

```cpp
// network/protocol_handler.cpp

void ProtocolHandler::sendRequest(const QString& cmd, const QByteArray& payload,
                                   ResponseCallback callback) {
    QString requestId = generateRequestId();
    
    if (callback) {
        m_pendingRequests[requestId] = callback;
    }
    
    // 构造 ClientMessage
    swift::gate::ClientMessage msg;
    msg.set_cmd(cmd.toStdString());
    msg.set_payload(payload.data(), payload.size());
    msg.set_request_id(requestId.toStdString());
    
    std::string serialized;
    msg.SerializeToString(&serialized);
    
    emit dataToSend(QByteArray::fromStdString(serialized));
}

void ProtocolHandler::handleMessage(const QByteArray& data) {
    swift::gate::ServerMessage msg;
    msg.ParseFromArray(data.data(), data.size());
    
    QString cmd = QString::fromStdString(msg.cmd());
    QString requestId = QString::fromStdString(msg.request_id());
    
    // 如果是响应
    if (!requestId.isEmpty() && m_pendingRequests.count(requestId)) {
        auto callback = m_pendingRequests[requestId];
        m_pendingRequests.erase(requestId);
        callback(msg.code(), QByteArray(msg.payload().data(), msg.payload().size()));
        return;
    }
    
    // 如果是推送通知
    if (cmd == "chat.new_message") {
        swift::gate::NewMessageNotify notify;
        notify.ParseFromString(msg.payload());
        emit newMessageNotify(notify);
    } else if (cmd == "chat.recall") {
        // ...
    }
}
```

#### Step 3: 登录界面

```cpp
// ui/loginwindow.cpp

void LoginWindow::onLoginClicked() {
    QString username = ui->usernameEdit->text();
    QString password = ui->passwordEdit->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter username and password");
        return;
    }
    
    // 先连接 WebSocket
    m_wsClient->connect(Settings::instance().serverUrl());
}

void LoginWindow::onWebSocketConnected() {
    // 发送登录请求
    swift::auth::LoginRequest req;
    req.set_username(ui->usernameEdit->text().toStdString());
    req.set_password(ui->passwordEdit->text().toStdString());
    
    std::string payload;
    req.SerializeToString(&payload);
    
    m_protocol->sendRequest("auth.login", QByteArray::fromStdString(payload),
        [this](int code, const QByteArray& payload) {
            if (code == 0) {
                swift::auth::LoginResponse resp;
                resp.ParseFromArray(payload.data(), payload.size());
                
                // 保存登录信息
                Settings::instance().saveLoginInfo(
                    QString::fromStdString(resp.user_id()),
                    QString::fromStdString(resp.token())
                );
                
                // 跳转到主界面
                emit loginSuccess();
            } else {
                QMessageBox::warning(this, "Error", "Login failed");
            }
        }
    );
}
```

#### Step 4: 聊天界面

```cpp
// ui/chatwidget.cpp

void ChatWidget::onSendClicked() {
    QString content = ui->inputEdit->toPlainText().trimmed();
    if (content.isEmpty()) return;
    
    // 构造发送消息请求
    swift::chat::SendMessageRequest req;
    req.set_from_user_id(m_currentUserId.toStdString());
    req.set_to_id(m_chatId.toStdString());
    req.set_chat_type(m_chatType);
    req.set_content(content.toStdString());
    req.set_media_type("text");
    
    std::string payload;
    req.SerializeToString(&payload);
    
    m_protocol->sendRequest("chat.send", QByteArray::fromStdString(payload),
        [this, content](int code, const QByteArray& payload) {
            if (code == 0) {
                // 添加到消息列表
                addMessage(content, true);
                ui->inputEdit->clear();
            }
        }
    );
}

void ChatWidget::onNewMessageReceived(const swift::gate::NewMessageNotify& notify) {
    // 检查是否是当前会话的消息
    if (QString::fromStdString(notify.chat_id()) == m_chatId) {
        addMessage(QString::fromStdString(notify.content()), false,
                   QString::fromStdString(notify.from_nickname()));
    }
}
```

### 11.3 打包发布

```bash
# Windows
cd build
cmake --build . --config Release

# 使用 windeployqt 收集依赖
windeployqt SwiftChat.exe

# 使用 NSIS 制作安装包
makensis installer.nsi
```

---

## 12. 阶段十：容器化与部署

### 12.1 构建 Docker 镜像

```bash
# 构建所有服务镜像
cd SwiftChatSystem

# AuthSvr
docker build -t swift/authsvr:latest -f backend/authsvr/Dockerfile .

# 其他服务类似...

# 或使用 docker-compose 一键构建
docker-compose build
```

### 12.2 部署到 Minikube

```bash
# 启动 Minikube
minikube start --driver=docker --memory=4096

# 使用 Minikube 的 Docker 环境
eval $(minikube docker-env)

# 构建镜像（直接在 Minikube 内）
docker build -t swift/authsvr:latest -f backend/authsvr/Dockerfile .
# ... 其他服务

# 部署
kubectl apply -k deploy/k8s/

# 查看状态
kubectl get pods -n swift-chat

# 获取客户端连接地址
minikube service gatesvr -n swift-chat --url
```

### 12.3 验证部署

```bash
# 查看所有服务状态
kubectl get all -n swift-chat

# 查看日志
kubectl logs -f deployment/authsvr -n swift-chat

# 端口转发（调试用）
kubectl port-forward svc/authsvr 9094:9094 -n swift-chat

# 测试
grpcurl -plaintext localhost:9094 swift.auth.AuthService/Register
```

### 12.4 服务发现与负载均衡

#### Headless Service 配置

```yaml
# deploy/k8s/authsvr-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: authsvr
  namespace: swift
spec:
  clusterIP: None          # Headless Service
  selector:
    app: authsvr
  ports:
    - name: grpc
      port: 9094
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: authsvr
  namespace: swift
spec:
  replicas: 3              # 多副本
  selector:
    matchLabels:
      app: authsvr
  template:
    metadata:
      labels:
        app: authsvr
    spec:
      containers:
        - name: authsvr
          image: swift/authsvr:latest
          ports:
            - containerPort: 9094
```

#### gRPC 客户端负载均衡

```cpp
// backend/common/include/swift/grpc_client.h

#include <grpcpp/grpcpp.h>

namespace swift {

/**
 * 创建支持负载均衡的 gRPC Channel
 * gRPC 会自动通过 DNS 发现所有 Pod，并做客户端负载均衡
 */
inline std::shared_ptr<grpc::Channel> CreateLBChannel(
    const std::string& service_name,
    const std::string& ns = "swift",
    int port = 9094) {
    
    // DNS 格式：dns:///service.namespace.svc.cluster.local:port
    std::string target = "dns:///" + service_name + "." + ns + 
                         ".svc.cluster.local:" + std::to_string(port);
    
    grpc::ChannelArguments args;
    
    // 启用客户端负载均衡（轮询策略）
    args.SetLoadBalancingPolicyName("round_robin");
    
    // DNS 刷新间隔
    args.SetInt(GRPC_ARG_DNS_MIN_TIME_BETWEEN_RESOLUTIONS_MS, 10000);
    
    return grpc::CreateCustomChannel(
        target,
        grpc::InsecureChannelCredentials(),
        args
    );
}

}  // namespace swift
```

#### 使用示例

```cpp
// ZoneSvr 初始化 RPC Client
bool AuthRpcClient::Init() {
    // 使用负载均衡 Channel
    channel_ = swift::CreateLBChannel("authsvr", "swift", 9094);
    stub_ = swift::auth::AuthService::NewStub(channel_);
    return true;
}

// 每次 RPC 调用自动负载均衡到不同 Pod
auto response = stub_->ValidateToken(&context, request, &response);
```

#### 负载均衡架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Kubernetes Cluster                        │
│                                                             │
│  Client ──→ Ingress/LoadBalancer                            │
│                    │                                         │
│                    ▼                                         │
│              ┌──────────┐                                   │
│              │ GateSvr  │ x N (K8s Service LB)              │
│              └────┬─────┘                                   │
│                   │                                          │
│                   ▼                                          │
│              ┌──────────┐                                   │
│              │ ZoneSvr  │ (gRPC Client LB)                  │
│              │          │                                   │
│              │ ┌──────┐ │     Headless Service              │
│              │ │Client│─┼──→ authsvr.swift.svc              │
│              │ └──────┘ │         │                         │
│              └──────────┘         ├─→ Pod-1                 │
│                                   ├─→ Pod-2                 │
│                                   └─→ Pod-3                 │
└─────────────────────────────────────────────────────────────┘
```

---

## 13. 测试与调试

### 13.1 单元测试

```bash
# 运行所有测试
cd build
ctest --output-on-failure

# 运行特定服务的测试
./backend/authsvr/test_auth_service
```

### 13.2 集成测试

```bash
# 启动所有服务（本地）
./scripts/start_all.sh

# 运行集成测试
./tests/integration/run_tests.sh
```

### 13.3 端到端测试

1. 启动两个客户端实例
2. 分别登录 alice 和 bob
3. 测试以下场景：
   - 发送文本消息
   - 发送图片
   - 消息撤回
   - 离线消息接收
   - 好友添加/删除

### 13.4 性能测试

```bash
# 使用 ghz 进行 gRPC 压测
ghz --insecure \
    --proto backend/authsvr/proto/auth.proto \
    --call swift.auth.AuthService.Login \
    -d '{"username":"test","password":"123456"}' \
    -c 100 -n 10000 \
    localhost:9094
```

---

## 14. 常见问题

### Q1: Proto 编译失败

```
错误：common.proto not found
```

**解决：** 检查 CMakeLists.txt 中的 `-I` 路径是否正确

### Q2: RocksDB 打开失败

```
错误：Failed to open RocksDB: Corruption: ...
```

**解决：** 删除数据目录重新创建，或检查磁盘权限

### Q3: gRPC 连接超时

```
错误：Deadline Exceeded
```

**解决：**
1. 检查服务是否启动
2. 检查端口是否正确
3. 检查防火墙设置

### Q4: WebSocket 连接失败

```
错误：Connection refused
```

**解决：**
1. 检查 GateSvr 是否启动
2. 检查 URL 格式：`ws://host:port/ws`
3. 检查跨域设置（如果有）

### Q5: 客户端构建失败（Qt）

```
错误：Qt5 not found
```

**解决：**
1. 安装 Qt5 开发包
2. 设置 `Qt5_DIR` 环境变量
3. 或在 CMake 中指定：`-DQt5_DIR=/path/to/qt5/lib/cmake/Qt5`

---

**文档版本**：v1.0  
**最后更新**：2026年2月2日
