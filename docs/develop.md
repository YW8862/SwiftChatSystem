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
15. [OnlineSvr 登录会话服务](#15-onlinesvr-登录会话服务)

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

### 2.5 服务配置文件

各服务通过 **配置文件 + 环境变量** 加载配置，配置项不写死在代码里，便于部署与多环境切换。

- **格式**：key=value，支持 `#` 注释、空行；键名不区分大小写。
- **位置**：项目根目录下 `config/` 中提供各服务的示例文件（如 `config/authsvr.conf.example`），使用时复制为 `xxx.conf` 并按需修改；建议将 `*.conf` 加入 .gitignore，敏感项（如 `jwt_secret`）优先用环境变量注入。
- **加载方式**：
  - 命令行传参：`./authsvr /path/to/authsvr.conf`
  - 或环境变量指定路径：`AUTHSVR_CONFIG=/path/to/authsvr.conf`（未传参时生效）
  - 未指定时默认使用当前目录下 `authsvr.conf` 等。
- **环境变量覆盖**：同名项可由环境变量覆盖，前缀与服务对应（如 `AUTHSVR_PORT`、`FILESVR_GRPC_PORT`）。详见 `config/README.md`。
- **公共实现**：解析与 env 覆盖由公共库 `swift::KeyValueConfig`（`backend/common/include/swift/config_loader.h`）统一实现；各服务 `internal/config/config.cpp` 仅调用 `LoadKeyValueConfig(path, "PREFIX_")` 并将键值填到本服务的配置结构体。

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
- 双缓冲交换策略（前台缓冲接收写入，满或定时与后台缓冲交换，后台线程批量消费）

**可复用性**：该双缓冲 + 后台线程模式与 **component/asynclogger** 中已实现的 `RingBuffer`（Push / SwapAndGet）+ `BackendThread` 一致，不仅适用于日志，也可复用于 **FileSvr 文件块异步写**、**ChatSvr 聊天记录异步写** 等「生产者入队、消费者批量写盘/写库」的场景，详见 [8.5 异步写设计方案](#85-异步写设计方案)。

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
- 后台线程定期从 RingBuffer 读取并写入 Sink（对应实现中为 `SwapAndGet` 取整批条目后 `ProcessEntries`）
- 使用条件变量避免忙等待
- 优雅关闭：确保缓冲区数据全部刷盘

该 **RingBuffer + BackendThread** 模式与双缓冲队列一致，可复用于其他异步写场景（见 [8.5 异步写设计方案](#85-异步写设计方案)）。

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

AuthSvr 是最基础的服务，负责**身份与用户资料**；**登录会话**由 OnlineSvr 独立提供，ZoneSvr 登录/登出直接走 OnlineSvr。

**分层说明：** 对外 API 由 Handler 层直接实现，无独立 API 层。结构为 Handler（gRPC 接口）→ Service（业务逻辑）→ Store（数据持久化）。

**登录流程（ZoneSvr）：** 1）AuthSvr.VerifyCredentials(username, password) → user_id、profile；2）OnlineSvr.Login(user_id, device_id, device_type) → token。登出：OnlineSvr.Logout(user_id, token)。Token 校验：OnlineSvr.ValidateToken(token)。

### 5.1 功能清单

- [x] 用户注册
- [x] VerifyCredentials（校验用户名密码，返回 user_id 与 profile，供登录时先校验再调 OnlineSvr.Login）
- [x] 获取/更新用户资料
- [x] 密码加密存储
- 登录/登出/ValidateToken 统一走 **OnlineSvr**，AuthSvr 不再提供

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

#### Step 2: AuthSvr 无 SessionStore / JWT

**当前设计：** AuthSvr 仅负责身份与资料，**不**包含 SessionStore 与 JWT。

- **登录会话**（SessionStore、单设备策略、Token 签发）在 **OnlineSvr** 实现，见 [15. OnlineSvr 登录会话服务](#15-onlinesvr-登录会话服务)。
- **JWT 工具**（HS256）在 **backend/common** 的 `swift/jwt_helper.h`、`jwt_helper.cpp` 中统一实现，供 OnlineSvr 等使用。

AuthSvr 的 AuthService 仅依赖 **UserStore**，提供：Register、VerifyCredentials、GetProfile、UpdateProfile。

#### Step 3: 实现 AuthService（身份与资料）

```cpp
// backend/authsvr/internal/service/auth_service.h

AuthService::AuthService(std::shared_ptr<UserStore> store);  // 仅 UserStore，无 SessionStore

AuthService::RegisterResult AuthService::Register(username, password, nickname, email, avatar_url);
AuthService::VerifyCredentialsResult AuthService::VerifyCredentials(username, password);
std::optional<UserProfile> AuthService::GetProfile(user_id);
AuthService::UpdateProfileResult AuthService::UpdateProfile(user_id, nickname, avatar_url, signature);
```

关键行为：

| 功能 | 说明 |
|------|------|
| Register | 校验用户名、生成 user_id、密码加盐哈希、写入 UserStore |
| VerifyCredentials | 校验用户名密码，返回 user_id 与 profile（供 ZoneSvr 后接 OnlineSvr.Login） |
| GetProfile / UpdateProfile | 读写 UserStore |

#### Step 4: 实现 Handler（对外 API）

Handler 层直接实现 gRPC 接口（Register、VerifyCredentials、GetProfile、UpdateProfile）。业务逻辑类命名为 `AuthServiceCore`，与 proto 生成的 `AuthService` 区分；用户资料用 `AuthProfile`，避免与 proto 的 `UserProfile` 重名。

```cpp
// backend/authsvr/internal/handler/auth_handler.h

#include <memory>
#include "auth.grpc.pb.h"   // 由 swift_proto 提供（build 生成）

namespace swift::auth {
class AuthServiceCore;  // 业务逻辑类

class AuthHandler : public AuthService::Service {
public:
    explicit AuthHandler(std::shared_ptr<AuthServiceCore> service);
    ~AuthHandler() override;

    ::grpc::Status Register(::grpc::ServerContext* context,
                            const ::swift::auth::RegisterRequest* request,
                            ::swift::auth::RegisterResponse* response) override;

    ::grpc::Status VerifyCredentials(::grpc::ServerContext* context,
                                      const ::swift::auth::VerifyCredentialsRequest* request,
                                      ::swift::auth::VerifyCredentialsResponse* response) override;

    ::grpc::Status GetProfile(::grpc::ServerContext* context,
                              const ::swift::auth::GetProfileRequest* request,
                              ::swift::auth::UserProfile* response) override;

    ::grpc::Status UpdateProfile(::grpc::ServerContext* context,
                                  const ::swift::auth::UpdateProfileRequest* request,
                                  ::swift::common::CommonResponse* response) override;

private:
    std::shared_ptr<AuthServiceCore> service_;
};
}
```

```cpp
// backend/authsvr/internal/handler/auth_handler.cpp

#include "auth_handler.h"
#include "../service/auth_service.h"

namespace swift::auth {

::grpc::Status AuthHandler::Register(::grpc::ServerContext* context,
                                      const ::swift::auth::RegisterRequest* request,
                                      ::swift::auth::RegisterResponse* response) {
    auto result = service_->Register(
        request->username(), request->password(),
        request->nickname(), request->email(), request->avatar_url());
    if (result.success) {
        response->set_code(0);
        response->set_user_id(result.user_id);
    } else {
        response->set_code(1);
        response->set_message(result.error);
    }
    return ::grpc::Status::OK;
}

::grpc::Status AuthHandler::VerifyCredentials(...) {
    auto result = service_->VerifyCredentials(request->username(), request->password());
    if (result.success) {
        response->set_code(0);
        response->set_user_id(result.user_id);
        if (result.profile) {
            auto* p = response->mutable_profile();
            p->set_user_id(result.profile->user_id);
            p->set_username(result.profile->username);
            // ... nickname, avatar_url, signature, gender, created_at
        }
    } else {
        response->set_code(1);
        response->set_message(result.error);
    }
    return ::grpc::Status::OK;
}

::grpc::Status AuthHandler::GetProfile(...) {
    auto profile = service_->GetProfile(request->user_id());
    if (!profile) return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "user not found");
    response->set_user_id(profile->user_id);
    // ... 其余字段
    return ::grpc::Status::OK;
}

::grpc::Status AuthHandler::UpdateProfile(...) {
    auto result = service_->UpdateProfile(
        request->user_id(), request->nickname(),
        request->avatar_url(), request->signature());
    response->set_code(result.success ? 0 : 1);
    if (!result.success) response->set_message(result.error);
    return ::grpc::Status::OK;
}
}
```

说明：proto 生成目录由 `swift_proto` 的 `target_include_directories` 提供，authsvr 链接 `swift_proto` 后即可包含 `auth.grpc.pb.h`。

#### Step 6: 实现 main.cpp

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
    
    // 初始化存储（仅 UserStore，无 SessionStore）
    auto user_store = std::make_shared<swift::auth::RocksDBUserStore>(config.rocksdb_path);
    
    // 初始化服务
    auto service = std::make_shared<swift::auth::AuthService>(user_store);
    
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
// backend/authsvr/internal/service/auth_service_test.cpp

#include "auth_service.h"
#include "../store/user_store.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>

class AuthServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto suffix = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        user_db_path_ = "/tmp/auth_service_user_" + suffix;
        store_ = std::make_shared<swift::auth::RocksDBUserStore>(user_db_path_);
        service_ = std::make_unique<swift::auth::AuthService>(store_);
    }
    void TearDown() override {
        service_.reset();
        store_.reset();
        std::filesystem::remove_all(user_db_path_);
    }
    std::string user_db_path_;
    std::shared_ptr<swift::auth::RocksDBUserStore> store_;
    std::unique_ptr<swift::auth::AuthService> service_;
};

TEST_F(AuthServiceTest, Register_Success) {
    auto result = service_->Register("alice", "password123", "Alice", "alice@example.com");
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.user_id.empty());
}

TEST_F(AuthServiceTest, VerifyCredentials_Success) {
    auto reg = service_->Register("alice", "password123", "Alice", "alice@example.com");
    ASSERT_TRUE(reg.success);
    auto result = service_->VerifyCredentials("alice", "password123");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.user_id, reg.user_id);
    ASSERT_TRUE(result.profile.has_value());
}

TEST_F(AuthServiceTest, VerifyCredentials_WrongPassword) {
    service_->Register("alice", "password123", "Alice", "");
    auto result = service_->VerifyCredentials("alice", "wrongpassword");
    EXPECT_FALSE(result.success);
}
```

#### 运行 main 并测试 gRPC

验证 `cmd/main.cpp` 启动的 AuthSvr 进程是否正常对外提供 gRPC 接口：

**方式一：脚本自动测（推荐）**

```bash
# 项目根目录执行；若无 bin/authsvr 会先 make build
chmod +x scripts/test_authsvr.sh
./scripts/test_authsvr.sh
```

脚本会：启动 AuthSvr（临时数据目录、默认 9094 端口）→ 用 grpcurl 依次调用 Register、VerifyCredentials、GetProfile、UpdateProfile → 检查 code=0 → 退出并清理。未安装 grpcurl 时仅启动服务并打印下方手动命令。

**方式二：手动测试**

```bash
# 1. 构建
make build

# 2. 启动（当前终端或后台）
export AUTHSVR_PORT=9094
export AUTHSVR_ROCKSDB_PATH=./data/auth
mkdir -p ./data/auth
./bin/authsvr

# 3. 另开终端，用 grpcurl 调用（需安装 grpcurl）
GRPCURL_OPTS=(-plaintext -import-path backend/common/proto -import-path backend/authsvr/proto -proto backend/authsvr/proto/auth.proto)

# 注册
grpcurl "${GRPCURL_OPTS[@]}" -d '{"username":"alice","password":"pass1234","nickname":"Alice"}' localhost:9094 swift.auth.AuthService/Register

# 校验凭证（登录用）
grpcurl "${GRPCURL_OPTS[@]}" -d '{"username":"alice","password":"pass1234"}' localhost:9094 swift.auth.AuthService/VerifyCredentials

# 获取资料（将 <user_id> 换成上一步返回的 user_id）
grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"<user_id>"}' localhost:9094 swift.auth.AuthService/GetProfile

# 更新资料
grpcurl "${GRPCURL_OPTS[@]}" -d '{"user_id":"<user_id>","nickname":"Alice Updated"}' localhost:9094 swift.auth.AuthService/UpdateProfile
```

**预期**：Register/VerifyCredentials 成功时返回 `"code":0` 及 `user_id`；GetProfile 返回 UserProfile；UpdateProfile 返回 `"code":0`。失败时 `code` 为错误码（如 105 用户已存在）、`message` 为错误说明。

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
3. **FriendHandler** - 对外 API（实现 gRPC 接口，无独立 API 层）
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

ChatSvr 是核心服务，负责消息的存储、查询、撤回以及群组管理。

### 7.0 统一会话模型

- **私聊**：抽象为仅两人的会话（type=private）。两好友之间**仅有一个**私聊会话，通过 `ConversationRegistry::GetOrCreatePrivateConversation(u1, u2)` 得到唯一 `conversation_id`（如 `p_<min_uid>_<max_uid>`），消息与历史均按该 id 存储。
- **群聊**：多人会话（type=group），`conversation_id` 即 `group_id`；同一批人（≥3）可建多个群。
- **消息**：统一按 `conversation_id` 存时间线、拉历史，不区分私聊/群聊。
- **群专属能力**：拉人、踢人、转让群主、设置管理员、群公告等仅在 type=group 时允许，接口内校验后否则返回 NOT_GROUP_CHAT。

### 7.1 功能清单

- [x] 发送消息
- [x] 消息撤回（2分钟内）
- [x] 获取历史消息
- [x] 离线消息队列
- [x] 消息搜索（客户端本地；服务端无搜索 RPC）
- [x] 已读回执
- [x] 会话管理
- [x] **群组**：创建群（至少 3 人，不允许 1 人或 2 人建群）、邀请成员（仅对当前不在群内的用户生效）、解散/退群/踢人/转让群主/设置管理员等

### 7.2 群组协议与规则

- **创建群聊**：`CreateGroupRequest` 中 `creator_id` 为创建者，`member_ids` 为邀请的初始成员；**创建者 + 去重后的 member_ids 至少 3 人**，不允许 1 人或 2 人建群；失败时返回错误码 `GROUP_MEMBERS_TOO_FEW`（517）。
- **邀请成员**：`InviteMembersRequest` 用于现有群邀请新成员；**仅对当前不在群内的用户生效**，已是成员的 user_id 会被跳过。

### 7.3 关键实现

#### ConversationRegistry（私聊 get-or-create）

```cpp
// 两好友间唯一私聊会话 id，确定性生成
std::string RocksDBConversationRegistry::GetOrCreatePrivateConversation(
    const std::string& user_id_1, const std::string& user_id_2) {
    std::string a = user_id_1, b = user_id_2;
    if (a > b) std::swap(a, b);
    std::string conversation_id = "p_" + a + "_" + b;
    std::string key = "conv_meta:" + conversation_id;
    std::string value;
    if (!db_->Get(rocksdb::ReadOptions(), key, &value).ok()) {
        // 首次：写入 type=private 标记
        db_->Put(rocksdb::WriteOptions(), key, R"({"type":"private"})");
    }
    return conversation_id;
}
```

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
    
    // 2. 添加到会话时间线索引（conversation_id = 私聊 get-or-create 的 id 或 group_id）
    std::string conversation_id = msg.conversation_id.empty() ? msg.to_id : msg.conversation_id;
    std::string timeline_key = fmt::format("chat:{}:{}:{}", 
        conversation_id, rev_ts(msg.timestamp), msg.msg_id);
    batch.Put(timeline_key, "");
    
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
- [x] 文件下载（HTTP，支持 Range 断点续传）
- [x] 文件元信息管理
- [x] MD5 秒传检测
- [x] **单文件大小限制**：仅接受 1GB 以内文件（发送与接收均受此限制，可在配置中修改 `max_file_size`）
- [x] **断点续传**：上传与下载在断网恢复后均可从断点继续，不重复传输
- [x] **发送文件即产生消息记录**：会话中会有一条「发送文件」消息，双方可见；上传完成后消息带 file_id/url，超时未续传则消息保留但标为「发送失败」
- [x] **上传会话 24h 过期**：断线后若 24 小时内未上线续传，该上传请求视为放弃，已上传数据删除，关联消息标为「发送失败（传输中断）」

### 8.2 发送文件与消息记录、24h 放弃、发送失败

**1. 发送文件在消息里有一条记录**

- 发送文件时，**先**在会话中创建一条消息（`SendMessage` 带 `media_type=file`、`content=文件名`、`file_size`），该条消息的 `status=上传中`（3），双方都能看到「对方正在发送文件」或「你发送了文件（上传中）」。
- 上传完成后，客户端调用 **CompleteFileMessage**(msg_id, file_id, file_url)，将该消息更新为正常（`status=0`）并写入 `media_url`，对方即可点击下载。

**2. 上传会话过期时间与放弃逻辑**

- 每次 **InitUpload** 创建的上传会话有一个**过期时间**（配置项 `upload_session_expire_seconds`，默认 **24 小时**）。
- 若在过期前未通过续传完成上传，则视为**放弃该次上传**：
  - FileSvr 删除该 `upload_id` 下已上传的**所有临时数据**；
  - 若 InitUpload 时带了 **msg_id**（关联的聊天文件消息），FileSvr 调用 ChatSvr 的 **MarkFileMessageFailed**(msg_id)，将该条消息的 `status` 置为 **发送失败(传输中断)**（4）。

**3. 放弃上传后，发送文件信息仍存在**

- **消息记录不删除**：该条「发送文件」消息仍保留在会话历史中。
- **状态可知**：双方都能看到这条消息处于「发送失败」——即因对方传输中断/超时未续传导致发送未完成。客户端可根据 `status=4` 展示「发送失败，传输未完成」等文案。

**推荐流程小结**

1. 客户端 **SendMessage**(media_type=file, content=文件名, file_size=…) → 得到 `msg_id`，消息 status=上传中。
2. 客户端 **InitUpload**(…, msg_id=msg_id) → 得到 `upload_id`、`expire_at`（默认 24h）。
3. 客户端 **UploadFile** 流式上传；断线则 **GetUploadState** → 续传。
4. 上传完成 → **CompleteFileMessage**(msg_id, file_id, file_url) → 消息变为正常、带可下载 URL。
5. 若 24h 内未续传完成 → FileSvr 定时清理该会话、删临时数据、调 **MarkFileMessageFailed**(msg_id) → 消息保留，status=发送失败。

### 8.3 文件大小限制与断点续传

**大小限制**

- 服务端配置项 `max_file_size` 默认 **1GB**（`1024*1024*1024` 字节）。
- `InitUpload`、`GetUploadToken`、流式上传首包中的 `file_size` 若超过该限制，将直接拒绝（返回错误码与说明）。
- 下载时不会产生大于 1GB 的文件（上传已限制），下载 URL 支持按 Range 分片拉取。

**上传断点续传流程**

1. （发文件时）先 **SendMessage**(media_type=file, content=文件名, file_size=…) 得到 `msg_id`，再 **InitUpload**(…, msg_id=msg_id) → 返回 `upload_id`、`expire_at`（默认 24h）。若 `file_size > max_file_size` 则返回错误。
2. 客户端通过 **UploadFile** 流式发送：首条消息为 **UploadMeta**（带 `upload_id`、file_name、file_size 等），之后按序发送 **chunk** 数据。
3. 若中途断网，连接恢复后：
   - 调用 **GetUploadState**(upload_id) → 得到已接收字节数 `offset` 及 `expire_at`（未过期可续传）。
   - 再次发起 **UploadFile** 流：首条为 **ResumeUploadMeta**(upload_id, offset)，然后从 `offset` 起继续发送 **chunk**。服务端追加写入，完成后落盘并返回 file_id；客户端再调 **CompleteFileMessage** 更新消息。
4. 若在 **expire_at** 之前都未续传完成，FileSvr 会放弃该上传、删除已传数据，并调用 **MarkFileMessageFailed**(msg_id)，消息保留且 status=发送失败。

**下载断点续传**

- 通过 **GetFileUrl** 获取下载 URL 后，使用 HTTP **Range** 头请求指定字节范围（如 `Range: bytes=0-1048575`）。
- 服务端 HTTP 下载接口需支持 **Range**：解析 `Range`，从文件 `offset` 起读取对应长度并返回 `206 Partial Content`；连接断开后，客户端可从下一段 range 继续请求，直至收齐整个文件。

**其他消息类型在断网恢复后的行为**

- 普通请求/响应（如聊天、好友、资料等）：客户端应使用 **request_id** 与响应中的 request_id 一一对应；断线重连后可根据业务需要**重发未确认的请求**，服务端保持幂等或去重即可。
- 大消息或需要可靠送达的场景：建议采用**分片 + 序号**的机制，断线恢复后只补传缺失片段，与文件断点续传思路一致。

### 8.4 关键实现

#### InitUpload 与上传大小校验、会话过期

在流式上传前必须先调用 `InitUpload`，服务端校验 `file_size <= config.max_file_size`（默认 1GB），通过则创建上传会话并返回 `upload_id` 和 `expire_at`（默认当前时间 + `upload_session_expire_seconds`，通常 24 小时）。可选传入 `msg_id` 以关联聊天文件消息，超时放弃时由 FileSvr 调 ChatSvr.MarkFileMessageFailed。后续 `UploadFile` 流的首条消息须携带该 `upload_id`（新传用 `UploadMeta`，续传用 `ResumeUploadMeta`）。定时任务需扫描过期未完成的会话：删除临时文件并若存在 `msg_id` 则调用 ChatSvr 更新消息状态为发送失败。

#### gRPC 流式上传（含断点续传）

```cpp
// 首条消息为 meta 或 resume_meta，之后为 chunk
grpc::Status FileServiceImpl::UploadFile(
    grpc::ServerContext* context,
    grpc::ServerReader<swift::file::UploadChunk>* reader,
    swift::file::UploadResponse* response) {
    
    swift::file::UploadChunk chunk;
    std::string upload_id;
    int64_t write_offset = 0;  // 新传为 0，续传由 ResumeUploadMeta 指定
    bool first = true;
    
    while (reader->Read(&chunk)) {
        if (chunk.has_meta()) {
            // 新上传：校验 upload_id、file_size <= max_file_size，打开临时文件
            upload_id = chunk.meta().upload_id();
            write_offset = 0;
            // ... 打开/创建 upload_id 对应临时文件，校验 file_size ...
        } else if (chunk.has_resume_meta()) {
            // 断点续传：校验 upload_id、offset 与已写入长度一致，seek 到 offset
            upload_id = chunk.resume_meta().upload_id();
            write_offset = chunk.resume_meta().offset();
            // ... 根据 GetUploadState 已记录的 offset 校验并 seek ...
        } else {
            // 数据块：从 write_offset 追加写入，并更新已写入长度（供 GetUploadState 查询）
            const auto& bytes = chunk.chunk();
            // ... 追加写入，write_offset += bytes.size() ...
        }
        first = false;
    }
    
    // 若总写入长度 == file_size，则落盘、生成 file_id、返回；否则保持未完成状态供续传
    // ...
    return grpc::Status::OK;
}
```

#### HTTP 下载服务（支持 Range 断点续传）

```cpp
// 解析 URL: /files/{file_id}，支持 Range: bytes=start-end 断点续传
void HttpServer::HandleRequest(
    http::request<http::string_body>& req,
    http::response<http::file_body>& res) {
    
    std::string target = req.target();
    if (!target.starts_with("/files/")) {
        res.result(http::status::not_found);
        return;
    }
    
    std::string file_id = target.substr(7);
    auto meta = file_store_->GetById(file_id);
    if (!meta) {
        res.result(http::status::not_found);
        return;
    }
    
    res.set(http::field::content_type, meta->content_type);
    res.set(http::field::content_disposition,
            "attachment; filename=\"" + meta->file_name + "\"");
    
    // 解析 Range 头，实现断点续传
    int64_t range_start = 0, range_end = meta->file_size - 1;
    if (auto range = req.find(http::field::range); range != req.end()) {
        // 解析 "bytes=0-1023" 等形式，设置 range_start/range_end
        // 若合法：res.result(http::status::partial_content); 并设置 Content-Range
        // 从 range_start 起读取 (range_end - range_start + 1) 字节到 res.body()
    } else {
        res.body().open(meta->storage_path.c_str(), beast::file_mode::scan);
    }
    res.prepare_payload();
}
```

### 8.5 异步写设计方案

当前 FileSvr 的 `AppendChunk` 与 ChatSvr 的「写聊天记录」（如 `MessageStore::Save`）均在 gRPC 工作线程内**同步写盘/写库**，高并发或慢盘时可能占满线程池。下面给出**未来可选的**统一异步写设计方案，实现时可参考本项目中 AsyncLogger 的双缓冲队列（见 [3. 阶段一：AsyncLogger](#3-阶段一asynclogger-异步日志库) 与 [system.md 异步日志系统设计](system.md#9-异步日志系统设计asynclogger)），将「日志条目」替换为「写任务」即可复用同一套生产者/消费者模型。

**当前实现状态**：

- FileSvr、ChatSvr 等**直接同步写**（在 gRPC 工作线程内调用 Store/写盘），**不经过** `IWriteExecutor`，避免多余封装与数据拷贝。
- `backend/common` 中的「异步写中间层」接口 `swift::async::IWriteExecutor`（见 `swift/async_write.h`）及 `SyncWriteExecutor` 已实现但**暂无业务调用**，仅作预留：当引入真正的异步写实现（如 `AsyncDbWriteExecutor`）时，再由各 Svr 注入 executor 并改为通过 `SubmitAndWait` / `Submit` 提交写任务。

**未来可选策略**（切到 Redis + MySQL 或写成为瓶颈时）：

- 新增 `AsyncDbWriteExecutor`：内部**写队列 + 写线程池**，按 key 分片、批量写、有限重试、背压。
- 强一致 RPC（发消息、文件块上传）使用 `SubmitAndWait`（入队 + 阻塞等结果）；非核心写（埋点、统计）使用 `Submit` fire-and-forget。

#### 参考 AsyncLogger 双缓冲实现

本仓库 **component/asynclogger** 已实现「双缓冲 + 后台线程」的异步写模式，可直接参考并复用到文件/消息异步写：

| AsyncLogger 组件 | 作用 | 对应到 FileSvr / ChatSvr 异步写 |
|------------------|------|----------------------------------|
| **RingBuffer**（`ring_buffer.h/cpp`） | 前台缓冲区 `front_buffer_` 接收生产者 **Push**；消费者通过 **SwapAndGet** 与后台缓冲区交换，一次性取走整批条目；有界容量 + `condition_variable` 通知 | 将 `LogEntry` 换成「写任务」类型（如 `ChunkTask` / `SaveMessageTask`），Push 由 gRPC Handler 调用，SwapAndGet 由写线程调用，取到的一批任务按 key 分组后顺序写盘/写库 |
| **BackendThread**（`backend_thread.h/cpp`） | 循环调用 `buffer_.SwapAndGet(entries, timeout_ms)`，取到后 **ProcessEntries**（格式化并写 Sink） | 写线程循环 SwapAndGet 取写任务，按 `upload_id` 或 `conversation_id` 保证顺序，执行写临时文件/MessageStore::Save，写完后对需要「返回前落盘」的任务执行 **promise.set_value** 通知 Handler |

**适配要点**：

- **任务类型**：AsyncLogger 推的是 `LogEntry`；FileSvr 推 `ChunkTask`（upload_id + data + optional promise），ChatSvr 推 `SaveMessageTask`（MessageData + 会话/离线更新 + optional promise）。
- **完成通知**：若需「返回前已落盘/落库」，每个任务可带 `std::promise<Result>`，Worker 在 ProcessEntries 中处理完该任务后 `promise.set_value(...)`，Handler 侧 `future.get()` 再填 response。
- **顺序保证**：日志是整批交换后任意顺序写；文件/消息需**按 key 串行**。做法可以是：(1) 单写线程，自然保序；(2) 多写线程时按 `upload_id` / `conversation_id` 做分片，同一 key 始终进同一队列/同一 worker。
- **背压**：与 RingBuffer 一致，容量满时 Push 返回 false 或阻塞，Handler 可返回 `SERVICE_UNAVAILABLE` 或短暂等待。

具体代码结构可对照 `component/asynclogger/src/ring_buffer.cpp`（双缓冲交换、Notify、Stop）与 `backend_thread.cpp`（Run 循环、SwapAndGet、ProcessEntries）。

#### 目标与约束

- **目标**：将「写盘 / 写 DB」从 gRPC 工作线程挪到**专用写线程**，避免 I/O 阻塞事件循环。
- **约束**：
  - 同一上传会话内 chunk 必须**严格顺序**落盘（FileSvr）。
  - 消息写入顺序与请求顺序一致（ChatSvr），且返回客户端前需**已落库**（或明确「已提交」语义）。

#### 通用模型：写队列 + 写线程

```
                    gRPC 线程池                    写线程（或线程池）
  ┌─────────────────────────────────────┐     ┌─────────────────────────────┐
  │  Handler 收到请求                     │     │  Worker 循环：               │
  │    → 构造写任务（Chunk / Message）     │     │    从队列取任务               │
  │    → 投递到「按 key 分片的队列」       │     │    执行写盘/写 DB             │
  │    → 等待该任务的完成信号（可选）      │     │    更新进度/状态               │
  │    → 返回响应                         │     │    通知等待者（若有）          │
  └─────────────────────────────────────┘     └─────────────────────────────┘
            │ 入队 + 可选阻塞等待                          │
            └────────────────── 有界队列 ──────────────────┘
```

- **按 key 分片**：FileSvr 按 `upload_id` 分队列（或单队列内按 upload_id 严格顺序消费），保证同一上传内顺序；ChatSvr 可按 `conversation_id` 或全局单队列（消息本身带序号）。
- **有界队列**：队列满时 Handler 可阻塞或返回「服务忙」，避免内存爆掉。
- **完成信号**：若需「返回前已落盘/落库」，则任务带 `promise`/`future` 或回调，Worker 写完后 set_value/回调，Handler 在 `future.get()` 或回调里填 response 再返回。

---

#### FileSvr：文件块异步写

- **队列**：按 `upload_id` 分片（例如 `map<upload_id, queue<ChunkTask>>`），或全局单队列 + 任务带 `upload_id`，由 Worker 保证同一 `upload_id` 串行写。
- **ChunkTask**：`upload_id`、`data`（拷贝或 shared_ptr）、`expected_offset`（可选，用于校验顺序）。
- **流程**：
  1. Handler 在 `AppendChunk` 里：校验 session、构造 ChunkTask，投递到该 `upload_id` 对应队列。
  2. Handler **阻塞等待**该 chunk 的「写完成」future（或带 request_id 的 completion 回调）。
  3. Worker 从队列取该 upload_id 的下一个任务，追加写临时文件，flush，调用 `UpdateUploadSessionBytes(upload_id, new_offset)`，然后 notify 等待方。
  4. Handler 被唤醒后，用 `new_offset` 填 `AppendChunkResult` 并返回。
- **顺序**：同一 `upload_id` 单队列 + 单写线程，自然保证顺序；多写线程时需按 `upload_id` 路由到同一 worker。
- **背压**：队列设 max_size，满时 Handler 可返回 `SERVICE_UNAVAILABLE` 或短暂阻塞。

这样 **gRPC 线程只做入队 + 等待**，不执行 `write/flush`；写盘在 Worker 线程，与当前「同步写」语义一致（返回时该 chunk 已落盘且 session 已更新）。

---

#### ChatSvr：聊天记录异步写

- **同理**：发送消息时不再在 gRPC 线程内直接调 `MessageStore::Save` / `ConversationStore::Upsert` 等，而是：
  1. 生成 `msg_id`、构造 `MessageData`（及会话更新等）。
  2. 将「写任务」（SaveMessageTask：msg、会话更新、离线队列等）投递到**写队列**。
  3. Handler **阻塞等待**该任务的「写完成」信号（例如 `future.get()`）。
  4. 写线程从队列取任务，依次执行 `MessageStore::Save`、ConversationStore、离线队列等，完成后 notify。
  5. Handler 被唤醒后填 `SendMessageResponse`（msg_id、timestamp）并返回。
- **顺序**：单写线程时自然按入队顺序落库；多写线程时可按 `conversation_id` 分片，保证同一会话内顺序。
- **一致性**：返回给客户端时表示「已落库」，与当前同步写语义一致；若希望「先返回再异步写」，则需额外协议（如先返回 msg_id，写失败再推送发送失败），复杂度更高，一般不首选。

---

#### 实现要点小结

| 项目       | FileSvr 异步写              | ChatSvr 异步写                 |
|------------|-----------------------------|--------------------------------|
| 队列 key   | `upload_id`                 | 可选 `conversation_id` 或全局  |
| 任务内容   | chunk 数据 + 预期 offset    | MessageData + 会话/离线更新    |
| 完成语义   | 该 chunk 已 flush + session 已更新 | 该条消息已 Save + 会话/离线已更新 |
| Handler    | 入队 + 等 future 再返回     | 入队 + 等 future 再返回        |
| 背压       | 有界队列，满则拒绝或等待     | 同上                            |

两者都是**「入队 + 阻塞等写完成」**的异步写：I/O 在写线程，gRPC 线程不直接写盘/写库，但返回前数据已持久化，语义与当前同步实现一致，仅把「谁在执行写」从 gRPC 线程换成了写线程。

---

## 9. 阶段七：ZoneSvr 路由服务

ZoneSvr 是系统的 **API Gateway**（与 system.md 2.2 节 Zone-System 架构一致），统一持有所有后端服务的 RPC Client，负责请求转发和**消息路由**；实际业务逻辑在 AuthSvr、OnlineSvr、ChatSvr、FriendSvr、FileSvr 中实现。**登录/登出/ValidateToken 走 OnlineSvr**，AuthSvr 仅负责 VerifyCredentials、GetProfile、UpdateProfile。

### 9.1 架构与职责（对应 system.md 2.2）

```
                         ┌─────────────────────────────────────────────┐
                         │                  ZoneSvr                     │
                         │  ┌─────────────────────────────────────┐    │
                         │  │          SystemManager              │    │
 GateSvr ──gRPC──────────│──│  AuthSystem │ ChatSystem │ FriendSys │    │
                         │  │  GroupSystem │ FileSystem │ OnlineClient│   │
                         │  └─────────────────────────────────────┘    │
                         │                    │                         │
                         │           ┌────────▼────────┐                 │
                         │           │  SessionStore   │  ← 消息路由     │
                         │           │ (user→gate 映射)│                 │
                         │           └─────────────────┘                 │
                         └─────────────────────────────────────────────┘
                                              │
              ┌───────────────────────────────┼───────────────────────────────┐
              ▼                               ▼                               ▼
         AuthSvr(RPC)                   OnlineSvr(RPC)                  ChatSvr(RPC) ...
         (身份/资料)                     (登录会话/Token)                 (消息/群组)
```

**职责划分（与 system.md 一致）：**

| 组件 | 职责 |
|------|------|
| **SystemManager** | 管理所有 System 生命周期，注入 SessionStore 与 ZoneConfig，Init/Shutdown |
| **AuthSystem** | VerifyCredentials(AuthSvr) + Login/Logout/ValidateToken(OnlineSvr) |
| **ChatSystem** | 转发 ChatSvr RPC；PushToUser 时查 SessionStore → GateRpcClient 推送 |
| **FriendSystem / GroupSystem / FileSystem** | 对应后端 RPC 转发层 |
| **SessionStore** | 用户→Gate 映射（消息路由用）；登录会话在 OnlineSvr，此处仅连接路由 |

### 9.2 目录结构（与 system.md 11 节一致）

```
backend/zonesvr/
├── cmd/main.cpp
├── proto/zone.proto
└── internal/
    ├── config/config.h、config.cpp          # ZoneConfig，key=value + 环境变量
    ├── system/
    │   ├── base_system.h、auth_system.h/cpp、chat_system.h/cpp、
    │   │   friend_system.h/cpp、group_system.h/cpp、file_system.h/cpp
    │   └── system_manager.h/cpp
    ├── rpc/
    │   ├── rpc_client_base.h/cpp
    │   ├── auth_rpc_client.h/cpp、online_rpc_client.h/cpp
    │   ├── chat_rpc_client.h/cpp、friend_rpc_client.h/cpp、
    │   │   group_rpc_client.h/cpp、file_rpc_client.h/cpp
    │   └── gate_rpc_client.h/cpp            # 推送到 Gate
    ├── handler/zone_handler.h/cpp            # 实现 ZoneService gRPC
    ├── service/zone_service.h/cpp            # 业务编排，调用 SystemManager + SessionStore
    └── store/session_store.h/cpp             # MemorySessionStore / RedisSessionStore
```

### 9.3 功能清单与 proto 接口（zone.proto）

- [x] 配置：ZoneConfig（后端地址、session_store_type、redis_url 等），main 启动 gRPC 服务（端口 9092）
- [x] SessionStore：Memory 版 + Redis 版（Key 与 system.md 5.2、6.3 一致）
- [x] SystemManager：按配置创建 SessionStore、各 System，SetConfig/SetSessionStore 后 Init
- [x] 各 System Init：连接对应后端（auth_svr_addr、online_svr_addr、chat_svr_addr 等），实现 RPC 转发；Shutdown 时 Disconnect 并 reset
- [x] Gate 注册与心跳：GateRegister、GateHeartbeat（SessionStore 写 gate:*、gate:list）
- [x] 用户上线/下线：UserOnline、UserOffline（SessionStore 写 session:{user_id}）
- [x] ZoneHandler：实现 proto 的 ZoneService::Service，9 个 RPC 委托给 ZoneServiceImpl；main 组装 SystemManager → ZoneServiceImpl(store, &manager) → ZoneHandler → RegisterService
- [x] 接入认证：internal_secret 通过 SetAuthMetadataProcessor（InternalSecretProcessor）校验 x-internal-secret
- [x] 消息路由：RouteMessage/PushToUser 在 ZoneServiceImpl 中查 SessionStore，并通过 PushToGate 使用 GateRpcClient::PushMessage 将消息推送到对应 Gate
- [x] 广播、在线状态：Broadcast、GetUserStatus、KickUser 已实现；Broadcast 通过 SessionStore 获取在线用户并逐个 PushToGate，统计 online_count/delivered_count
- [x] **客户端业务请求按 cmd 分发**：已实现 `HandleClientRequest` RPC（zone.proto）；Gate 转发 cmd+payload，ZoneHandler 调用 ZoneServiceImpl.HandleClientRequest，按 cmd 分发到各 System；auth.* / chat.* / friend.* / group.* / file.* 等 cmd 已全部实现（见 9.6）

**ZoneService（zone.proto）与 system.md 6.2 对应：** UserOnline、UserOffline、RouteMessage、Broadcast、GetUserStatus、PushToUser、KickUser、GateRegister、GateHeartbeat。

### 9.3.1 实现步骤（推荐顺序）

以下步骤按依赖顺序排列，每步为可独立完成的一小块工作。

#### 一、配置与进程启动

| 步骤 | 细化工作 | 对应 system.md |
|------|----------|----------------|
| **Step 1** | **ZoneConfig 结构体**（config.h）：定义 `host`、`port`(默认 9092)、`auth_svr_addr`、`online_svr_addr`、`friend_svr_addr`、`chat_svr_addr`、`file_svr_addr`、`gate_svr_addr`、`session_store_type`、`redis_url`、`session_expire_seconds`、`gate_heartbeat_timeout`、`internal_secret`（可选，空表示不校验） | §2.6 |
| **Step 2** | **配置加载**（config.cpp）：`LoadConfig(path)` 使用 `swift::LoadKeyValueConfig(path, "ZONESVR_")` 读 key=value，环境变量覆盖；将键映射到 ZoneConfig 各字段；`internal_secret` 建议仅从 `ZONESVR_INTERNAL_SECRET` 读取 | §2.6 |
| **Step 3** | **main 启动 gRPC**（cmd/main.cpp）：解析配置文件路径（参数或环境变量）、调用 `LoadConfig`；创建 SystemManager、ZoneConfig 传入后 `manager.Init(config)`（后续 Step 完成后再接）；`grpc::ServerBuilder` 绑定 `host:port`、注册 ZoneService（Handler 后续 Step 实现）、`BuildAndStart()`、`Wait()` | §2.6 |

#### 二、接入认证（内网密钥）

| 步骤 | 细化工作 | 对应 system.md |
|------|----------|----------------|
| **Step 4** | **服务端拦截器**：实现 `InternalSecretInterceptor`（继承 `grpc::ServerInterceptor` 或使用 `ServerBuilder::AddInterceptor` 传入 lambda/仿函数）；在拦截逻辑中从 `ServerContext::client_metadata()` 读取 `x-internal-secret`；若 `config.internal_secret` 非空且 metadata 缺失或值不等，则 `Finish(Status(UNAUTHENTICATED, "missing or invalid x-internal-secret"))` 并终止，否则放行。**当前实现**：使用 `SetAuthMetadataProcessor(InternalSecretProcessor)` 在连接建立时校验，等价于“先认证再进业务”。 | §2.7 |
| **Step 5** | **注册拦截器**：在 main 中 `ServerBuilder::AddInterceptor(...)`（在 `RegisterService` 之前）；或通过 `creds->SetAuthMetadataProcessor(...)` 在 AddListeningPort 前注入。当前采用后者。 | §2.7 |

#### 三、SessionStore 接口与内存实现

| 步骤 | 细化工作 | 对应 system.md |
|------|----------|----------------|
| **Step 6** | **SessionStore 接口**（store/session_store.h）：定义 `UserSession`（user_id, gate_id, gate_addr, device_type, device_id, online_at, last_active_at）、`GateNode`（gate_id, address, current_connections, registered_at, last_heartbeat）；抽象接口 `SetOnline`/`SetOffline`/`GetSession`/`GetSessions`/`IsOnline`/`UpdateLastActive`、`RegisterGate`/`UnregisterGate`/`UpdateGateHeartbeat`/`GetGate`/`GetAllGates` | §4.2、§5.2 |
| **Step 7** | **MemorySessionStore 实现**（store/session_store.cpp）：内部 `Impl` 用 `unordered_map<string, UserSession>`、`unordered_map<string, GateNode>` + `shared_mutex`；实现上述全部接口；构造/析构、无 Redis 依赖 | §4.2 |

#### 四、System 层与 SystemManager

| 步骤 | 细化工作 | 对应 system.md |
|------|----------|----------------|
| **Step 8** | **BaseSystem**（system/base_system.h）：纯虚 `Name()`/`Init()`/`Shutdown()`；`SetSessionStore(shared_ptr)`、`SetConfig(const ZoneConfig*)`；protected 成员 `session_store_`、`config_`；前向声明 SessionStore、ZoneConfig | §2.2 |
| **Step 9** | **SystemManager**（system/system_manager.h/cpp）：成员 `session_store_`、`auth_system_`/`chat_system_`/`friend_system_`/`group_system_`/`file_system_`；`Init(ZoneConfig)` 内按 `session_store_type` 创建 `MemorySessionStore` 或 `RedisSessionStore(redis_url)`，创建各 System，对每个 System 调用 `SetSessionStore`、`SetConfig(&config)`，再依次 `system->Init()`；`GetAuthSystem()` 等 getter；`Shutdown()` 逆序关闭并清空 session_store_ | §2.2 |
| **Step 10** | **各 System Init 连接后端**（auth_system.cpp 等）：在 `Init()` 中若 `config_` 非空则创建对应 RPC Client（如 AuthRpcClient、OnlineRpcClient），`Connect(config_->auth_svr_addr)` 等，成功后 `InitStub()`；Shutdown 时 Disconnect 并 reset；ChatSystem 仅连 ChatSvr。**已实现**：Auth/Online/Chat/Friend/Group/File 各 RPC Client 的 stub 与关键方法已实现；AuthSystem 的 Login/Logout/ValidateToken 已调 RPC，Friend/Group/File 业务方法已接对应 Client。 | §2.2 |

#### 五、ZoneService 与 ZoneHandler（gRPC 入口）

| 步骤 | 细化工作 | 对应 system.md |
|------|----------|----------------|
| **Step 11** | **ZoneServiceImpl 类**（service/zone_service.h/cpp）：业务类命名为 ZoneServiceImpl（与 proto 生成的 ZoneService 区分）。构造函数接收 `shared_ptr<SessionStore>`、`SystemManager*`（可选）；实现 `UserOnline`/`UserOffline`/`GateRegister`/`GateHeartbeat`/`GetUserSession`/`GetUserStatuses`/`RouteToUser`/`Broadcast`/`KickUser`；RouteToUser/Broadcast 中已通过 PushToGate 调用 GateRpcClient::PushMessage 推送到对应 Gate。**已实现**。 | §2.2、§6.3 |
| **Step 12** | **ZoneHandler**（handler/zone_handler.h/cpp）：继承 proto 生成的 `ZoneService::Service`；9 个 RPC 从 request 解析参数、调用 `ZoneServiceImpl` 对应方法、写 response、返回 Status::OK；构造注入 `shared_ptr<ZoneServiceImpl>`。**已实现**。 | §2.2 |
| **Step 13** | **main 组装**：先 `SystemManager manager; manager.Init(config)`；再 `zone_svc = make_shared<ZoneServiceImpl>(manager.GetSessionStore(), &manager)`；`handler = make_shared<ZoneHandler>(zone_svc)`；creds 设置 SetAuthMetadataProcessor 后 AddListeningPort；RegisterService(handler.get())；BuildAndStart；退出前 manager.Shutdown()。**已实现**。 | §2.2 |

#### 六、Gate 注册与心跳（与 Store 对接）

| 步骤 | 细化工作 | 对应 system.md |
|------|----------|----------------|
| **Step 14** | **GateRegister 完整逻辑**：从 `GateRegisterRequest` 取 gate_id、address、current_connections；构造 `GateNode`，调用 `session_store_->RegisterGate(node)`。**已实现**（ZoneServiceImpl::RegisterGate）；Redis 版在 RedisSessionStore 中实现。 | §6.3 |
| **Step 15** | **GateHeartbeat 完整逻辑**：从 `GateHeartbeatRequest` 取 gate_id、current_connections；调用 `session_store_->UpdateGateHeartbeat(gate_id, connections)`。**已实现**（ZoneHandler → ZoneServiceImpl::GateHeartbeat）。 | §6.3 |

#### 七、消息路由（RouteMessage / PushToUser）

| 步骤 | 细化工作 | 对应 system.md |
|------|----------|----------------|
| **Step 16** | **GateRpcClient**（rpc/gate_rpc_client）：支持按 `gate_addr` 发起调用（可每次创建 channel 或按 addr 缓存 stub）；实现 `PushMessage(user_id, cmd, payload)`，对应 Gate 侧 PushMessage RPC；在 ZoneSvr 内由 ZoneServiceImpl 使用，按 gate_addr 缓存/创建 stub 调用不同 Gate。**已实现**。 | §6.2 |
| **Step 17** | **RouteMessage / PushToUser 实现**：从 SessionStore `GetSession(to_user_id)`；若为空则返回 delivered=false；若在线则取 `session.gate_addr`，用 GetOrCreateGateClient 取得 GateRpcClient 后调用 PushMessage；ChatSystem::PushToUser 通过 BindChatPushToUser 注入的 RouteToUser 回调投递。**已实现**。 | §6.2、§10 |

#### 八、Redis 版 SessionStore

| 步骤 | 细化工作 | 对应 system.md |
|------|----------|----------------|
| **Step 18** | **RedisSessionStore 实现**（store/session_store.cpp）：依赖 hiredis 或 redis-plus-plus；构造函数接收 `redis_url`，建立连接；`SetOnline` 用 HSET 写 `session:{user_id}` 各字段、EXPIRE 为 session_expire_seconds；`SetOffline` 用 DEL；`GetSession` 用 HGETALL 解析为 UserSession；RegisterGate 写 `gate:{gate_id}`、SADD gate:list；GateHeartbeat 更新 gate 字段并 EXPIRE gate:{gate_id} 60；UnregisterGate 用 DEL + SREM gate:list；GetGate/GetAllGates 按 Key 设计实现 | §5.1、§5.2、§6.3 |

### 9.4 System 基类设计

```cpp
// internal/system/base_system.h
// System 是 RPC 转发层，实际业务逻辑在后端服务

class BaseSystem {
public:
    virtual ~BaseSystem() = default;

    virtual std::string Name() const = 0;
    virtual bool Init() = 0;
    virtual void Shutdown() = 0;

    void SetSessionStore(std::shared_ptr<SessionStore> store);
    void SetConfig(const ZoneConfig* config);   // Init 前注入，用于 RPC 地址

protected:
    std::shared_ptr<SessionStore> session_store_;
    const ZoneConfig* config_ = nullptr;
};
```

各子类在 `Init()` 内根据 `config_` 的 `auth_svr_addr`、`online_svr_addr`、`chat_svr_addr` 等连接对应 RPC Client 并 InitStub。

### 9.5 SystemManager 与配置注入

SystemManager::Init(ZoneConfig) 内按 system.md 完成：① 按 `session_store_type` 创建 `MemorySessionStore` 或 `RedisSessionStore(redis_url)`；② 创建各 System；③ SetSessionStore、SetConfig(&config)；④ 依次各 System->Init()（连接后端 RPC）。

```cpp
// 初始化
SystemManager manager;
manager.Init(config);

// 获取 System，转发请求到后端服务
auto* auth = manager.GetAuthSystem();
auto response = auth->ValidateToken(request);  // → OnlineSvr RPC

auto* chat = manager.GetChatSystem();
auto result = chat->SendMessage(request);      // → ChatSvr RPC

// 消息路由（ZoneSvr 特有职责）
chat->PushToUser(user_id, "chat.new_message", payload);  // 查 SessionStore → GateRpcClient

// 关闭
manager.Shutdown();
```

### 9.6 请求处理流程

**设计原则：请求从 GateSvr 转到 ZoneSvr，再按 cmd 分发到各 System，各 System 通过 gRPC 调用后端业务逻辑。**

#### 业务请求链路（Gate → Zone → System → 后端 gRPC）

```
Client (WebSocket)
    │  cmd + payload（如 auth.login、chat.send_message）
    ▼
GateSvr  收到客户端消息
    │  gRPC 调用 ZoneSvr 统一入口（如 HandleClientRequest）
    ▼
ZoneSvr (ZoneHandler)
    │  根据 cmd 选择对应 System
    ├── "auth.*"   → AuthSystem  ──gRPC──→ AuthSvr / OnlineSvr（业务逻辑）
    ├── "chat.*"   → ChatSystem  ──gRPC──→ ChatSvr（业务逻辑）
    ├── "friend.*" → FriendSystem ──gRPC──→ FriendSvr（业务逻辑）
    ├── "group.*"  → GroupSystem  ──gRPC──→ ChatSvr/GroupService（业务逻辑）
    └── "file.*"   → FileSystem   ──gRPC──→ FileSvr（业务逻辑）
    │
    ▼  聚合结果
ZoneSvr 返回给 GateSvr → Client
```

- **ZoneSvr**：统一入口，只做会话/路由与 **按 cmd 分发到 System**，不实现业务逻辑。
- **各 System**：持有对应后端 RPC Client，将请求 **通过 gRPC 转发到 AuthSvr、ChatSvr、FriendSvr、FileSvr 等**，由后端完成业务与存储。
- **后端 Svr**：实际业务逻辑与数据落库。

**已实现**：zone.proto 中已增加 `HandleClientRequest(HandleClientRequestRequest) returns (HandleClientRequestResponse)`；ZoneHandler 将请求交给 ZoneServiceImpl.HandleClientRequest，ZoneServiceImpl 通过 `manager_` 按 cmd 调用各 System 再经 gRPC 调后端。当前已支持：**auth.***（login、logout、validate_token）；**chat.***（send_message、mark_read、pull_offline、recall_message、get_history、sync_conversations、delete_conversation）；**friend.***（add、handle_request、remove、block、unblock、get_friends、get_friend_requests）；**group.***（create、dismiss、invite_members、remove_member、leave、get_info、get_members、get_user_groups）；**file.***（get_upload_token、get_file_url、delete）。

**HandleClientRequest 约定（auth.*）：**

| cmd | 请求 payload（proto） | 响应 payload（proto） |
|-----|------------------------|------------------------|
| auth.login | AuthLoginPayload (username, password, device_id, device_type) | AuthLoginResponsePayload (success, user_id, token, expire_at, error) |
| auth.logout | AuthLogoutPayload (user_id, token) | 无（code/message 在 response 上） |
| auth.validate_token | AuthValidateTokenPayload (token) | AuthValidateTokenResponsePayload (user_id，空表示无效) |

**HandleClientRequest 约定（chat.*，已实现）：**

| cmd | 请求 payload（proto，均定义于 zone.proto） | 响应 payload（proto 或空） |
|-----|--------------------------------------------|----------------------------|
| chat.send_message | ChatSendMessagePayload (from_user_id, to_id, chat_type, content, media_url, media_type, client_msg_id, file_size, **mentions**, **reply_to_msg_id**) | ChatSendMessageResponsePayload (success, msg_id, timestamp, error) |
| chat.mark_read | ChatMarkReadPayload (chat_id, chat_type, last_msg_id)；user_id 由连接绑定与 token 决定 | 无单独 payload（code/message 在 HandleClientRequestResponse 上）；成功后由 ZoneSvr 向会话内其他人推送 `cmd="chat.read_receipt"`，payload 为 `ReadReceiptNotify`（见 gate.proto） |
| chat.pull_offline | ChatPullOfflinePayload (limit, cursor)；user_id 来自连接绑定 | ChatPullOfflineResponsePayload：`repeated ChatMessagePushPayload messages`（与实时 `chat.message` 推送格式一致）+ next_cursor + has_more |
| chat.recall_message | ChatRecallMessagePayload (msg_id, user_id) | 无 payload（code/message 在 response 上） |
| chat.get_history | ChatGetHistoryPayload (chat_id, chat_type, before_msg_id, limit)；user_id 来自连接 | ChatGetHistoryResponsePayload：repeated ChatMessagePushPayload messages + has_more |
| chat.sync_conversations | ChatSyncConversationsPayload (last_sync_time)；user_id 来自连接 | ChatSyncConversationsResponsePayload：repeated ChatConversationPayload conversations |
| chat.delete_conversation | ChatDeleteConversationPayload (chat_id, chat_type)；user_id 来自连接 | 无 payload（code/message 在 response 上） |

#### 当前已实现的路径（会话/路由/Gate 管理）

GateSvr 调用 ZoneSvr 的 9 个 RPC（UserOnline、UserOffline、RouteMessage、Broadcast、GetUserStatus、PushToUser、KickUser、GateRegister、GateHeartbeat）→ ZoneHandler → ZoneServiceImpl → SessionStore（及 PushToGate 时用 manager）。这类请求**不经过**各业务 System，仅维护会话与路由状态。各 System 及其 gRPC 转发已实现，可供上述业务入口复用。

**ZoneHandler 各 RPC 作用简述：**

| RPC | 作用 | 当前走向 |
|-----|------|----------|
| UserOnline | Gate 上报用户上线 | ZoneServiceImpl → SessionStore.SetOnline |
| UserOffline | Gate 上报用户下线 | ZoneServiceImpl → SessionStore.SetOffline |
| RouteMessage | 路由单条消息到用户 | ZoneServiceImpl.RouteToUser（查 session → PushToGate 调 GateRpcClient::PushMessage） |
| Broadcast | 广播到多用户 | ZoneServiceImpl.Broadcast（返 online_count/delivered_count） |
| GetUserStatus | 批量查在线状态 | ZoneServiceImpl.GetUserSession 逐用户 |
| PushToUser | 主动推送给指定用户 | ZoneServiceImpl.RouteToUser |
| KickUser | 踢用户下线 | ZoneServiceImpl.KickUser → SetOffline |
| GateRegister | Gate 注册 | ZoneServiceImpl → SessionStore.RegisterGate |
| GateHeartbeat | Gate 心跳 | ZoneServiceImpl → SessionStore.UpdateGateHeartbeat |

### 9.7 消息路由与 Gate 注册心跳（对应 system.md 6.2、6.3）

**消息路由流程：**

1. Client A → GateSvr-1 (发送消息)
2. GateSvr-1 → ZoneSvr（RouteMessage 或业务先 ChatSvr.SendMessage 再 PushToUser）
3. ZoneSvr 查 SessionStore：用户 B 在哪个 Gate？→ gate_addr
4. ZoneSvr → GateRpcClient.PushMessage(gate_addr, user_id=B, cmd, payload)
5. GateSvr-2 → Client B (WebSocket 推送)

**Gate 注册与心跳（system.md 6.3）：**

- Gate 启动：调用 ZoneSvr.GateRegister(gate_id, address, current_connections)；ZoneSvr 写 Redis `gate:{gate_id}`、`SADD gate:list {gate_id}`。
- Gate 心跳（建议每 30s）：ZoneSvr.GateHeartbeat(gate_id, connections)；ZoneSvr 更新 `EXPIRE gate:{gate_id} 60`。
- 用户上线：Gate 在 WS 连接并校验 Token 后调用 ZoneSvr.UserOnline(user_id, gate_id, device_type, device_id)；ZoneSvr 写 `session:{user_id}`，TTL 与配置一致（如 3600）。

### 9.8 关键实现

#### 9.8.1 ZoneSvr 接入认证（内网密钥，对应 system.md 2.7）

**目的**：只处理来自 GateSvr（或其它持有内网密钥的调用方）的请求，避免外部或伪造 gRPC/HTTP 直接调用 ZoneSvr。

**配置**：`ZoneConfig` 增加 `std::string internal_secret`；空字符串表示不校验（仅建议开发环境）。生产环境通过环境变量 `ZONESVR_INTERNAL_SECRET` 注入，不写入 .conf。

**ZoneSvr 服务端拦截器**：在创建 `grpc::Server` 时通过 `ServerBuilder::AddInterceptor` 注册拦截器；在 `InterceptUnaryUnary`（及流式等价）中从 `ServerContext::client_metadata()` 读取 key `x-internal-secret`，若 `internal_secret` 非空且 metadata 中缺失或值不一致，则调用 `context->Finish(grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "missing or invalid x-internal-secret"))` 并返回，否则调用 `continuation(context, request, response)` 继续业务。

```cpp
// 伪代码：ZoneSvr 服务端拦截器
class InternalSecretInterceptor : public grpc::ServerInterceptor {
public:
    explicit InternalSecretInterceptor(const std::string& expected_secret)
        : expected_(expected_secret) {}
    void Intercept(grpc::InterceptorBatchMethods* methods) override {
        if (expected_.empty()) { methods->Proceed(); return; }
        auto* ctx = methods->GetServerContext();
        auto it = ctx->client_metadata().find("x-internal-secret");
        if (it == ctx->client_metadata().end() ||
            std::string(it->second.data(), it->second.size()) != expected_) {
            methods->FailWithError(grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                                               "missing or invalid x-internal-secret"));
            return;
        }
        methods->Proceed();
    }
private:
    std::string expected_;
};
```

**GateSvr 调用 ZoneSvr 时携带密钥**：每次发起 gRPC 调用前在 `ClientContext` 上设置 metadata，例如 `context->AddMetadata("x-internal-secret", config.zonesvr_internal_secret)`；若使用统一的 ZoneSvr 客户端封装，可在封装层为每次调用注入该 metadata，避免遗漏。GateSvr 的 `internal_secret` 建议通过环境变量 `GATESVR_ZONESVR_INTERNAL_SECRET` 注入，与 ZoneSvr 的 `ZONESVR_INTERNAL_SECRET` 保持一致。

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

#### 消息路由（ZoneService 或 ChatSystem 调用）

RouteMessage/PushToUser 可由 ZoneService 统一实现（查 SessionStore + PushToGate），或 ChatSystem 在转发 SendMessage 后自行 PushToUser。二者均需 GateRpcClient 按 gate_addr 创建 channel 并调用 Gate 的 PushMessage。

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

GateSvr 是客户端的**接入点**（与 system.md 3、6 节一致）：**WebSocket** 面向客户端，**gRPC** 面向 ZoneSvr；负责连接管理、协议解析、请求转发与推送。多实例时由 LoadBalancer 分流，各 Gate 向 ZoneSvr 注册并心跳，用户上线时通知 ZoneSvr 写入 SessionStore。

### 10.1 架构与端口（对应 system.md 2.1、7.4）

| 协议 | 端口 | 用途 |
|------|------|------|
| WebSocket | 9090 | 客户端连接，二进制 Protobuf（ClientMessage/ServerMessage） |
| gRPC | 9091 | 供 ZoneSvr 调用：PushMessage（推送消息到本 Gate 某用户）、可选 Gate 状态查询 |

**职责：** 连接管理、协议解析（gate.proto）、心跳检测、将客户端 cmd 转发到 ZoneSvr、接收 ZoneSvr 的 PushMessage 并推送到对应 WebSocket。

### 10.2 目录结构（与 system.md 11 节一致）

```
backend/gatesvr/
├── cmd/main.cpp
├── proto/gate.proto
└── internal/
    ├── config/config.h、config.cpp       # 监听端口、ZoneSvr 地址等
    ├── handler/                          # gRPC 服务端，实现 GateInternalService（PushMessage）
    ├── service/gate_service.h/cpp        # 连接表、BindUser、PushToUser、转发到 ZoneSvr
    └── websocket/ 或 net/                # Boost.Beast WebSocket 监听、Session 生命周期
```

### 10.3 功能清单与 proto（gate.proto）

- [x] 配置：监听地址、WebSocket 端口(9090)、gRPC 端口(9091)、ZoneSvr 地址、**zonesvr_internal_secret**（与 ZoneSvr 一致，用于调用 ZoneSvr 时携带，见 system.md 2.7）；main 启动双端口
- [x] WebSocket 服务器（Boost.Beast）：接受连接、解析 ClientMessage、按 cmd 分发
- [x] 连接管理：conn_id ↔ user_id 绑定；断开时通知 ZoneSvr UserOffline
- [x] auth.login：客户端带 token/device_id/device_type；Gate 调 ZoneSvr.ValidateToken（或 OnlineSvr），成功后 BindUser，并调用 ZoneSvr.UserOnline(user_id, gate_id, device_type, device_id)
- [x] heartbeat：维持连接，可选带 client_time，回包 server_time
- [x] 业务 cmd（chat.*、friend.*、group.*、file.* 等）：转发到 ZoneSvr.HandleClientRequest(cmd, payload)，将响应通过 WebSocket 回给客户端
- [x] gRPC PushMessage：ZoneSvr 调用时根据 user_id 查本 Gate 连接，推送 ServerMessage 到对应 WebSocket

**gate.proto：** ClientMessage/ServerMessage、ClientLoginRequest、HeartbeatRequest/Response、NewMessageNotify 等（见 proto 文件）；服务端需实现 GateInternalService.PushMessage。

### 10.3.1 实现步骤（推荐顺序）

| 步骤 | 内容 | 对应 system.md |
|------|------|----------------|
| **Step 1** | 配置与 main：WebSocket 9090、gRPC 9091、zonesvr_addr、**zonesvr_internal_secret**（环境变量 GATESVR_ZONESVR_INTERNAL_SECRET）；启动 WebSocket 监听 + gRPC 服务 | §2.6、§2.7、§7.4 |
| **Step 2** | WebSocket 服务器（Boost.Beast）：accept、async_read、async_write、close；每连接生成 conn_id，维护 Session | §3 GateSvr |
| **Step 3** | 连接表：conn_id → (user_id, device_id, device_type)；BindUser(conn_id, user_id)、GetConnByUserId、OnDisconnect 清理并通知 ZoneSvr UserOffline | §6.3 |
| **Step 4** | 协议解析：读取二进制 → ClientMessage；根据 cmd 分发到 HandleLogin、HandleHeartbeat、ForwardToZone | §2 客户端协议 |
| **Step 5** | ZoneSvr gRPC 客户端：HandleRequest、UserOnline、UserOffline、GateRegister；**每次调用在 ClientContext 上 AddMetadata("x-internal-secret", zonesvr_internal_secret)**（与 ZoneSvr 2.7 认证一致） | §6.2、§6.3、§2.7 |
| **Step 6** | auth.login：解析 ClientLoginRequest，调用 ZoneSvr 或直接 OnlineSvr.ValidateToken；成功则 BindUser + ZoneSvr.UserOnline，并返回 ServerMessage | §10 核心流程 |
| **Step 7** | gRPC 服务端实现 PushMessage：根据 user_id 查本机连接，组 ServerMessage 并 async_write 到对应 WebSocket；心跳超时断开并 UserOffline | §6.2 |

### 10.4 关键实现

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
    
    // 校验 Token：调用 ZoneSvr（转发到 OnlineSvr.ValidateToken）或直接 OnlineSvr.ValidateToken
    std::string user_id = ValidateTokenAndGetUserId(req.token());
    if (user_id.empty()) {
        SendResponse(conn_id, "auth.login", request_id, 401, "invalid token");
        return;
    }
    
    BindUser(conn_id, user_id);
    // 通知 ZoneSvr 用户上线（system.md 6.3）
    zone_client_->UserOnline(user_id, gate_id_, req.device_type(), req.device_id());
    
    SendResponse(conn_id, "auth.login", request_id, 0, "");
}
```

**与 ZoneSvr 的协作：** Gate 启动时调用 ZoneSvr.GateRegister(gate_id, grpc_address)；定时调用 GateHeartbeat(gate_id, current_connections)；用户断开时调用 UserOffline(user_id, gate_id)。详见 develop.md 9.7、system.md 6.3。

**调用 ZoneSvr 时携带内网密钥（system.md 2.7）：** 每次向 ZoneSvr 发起 gRPC 调用前，在 `ClientContext` 上设置 metadata，例如 `context->AddMetadata("x-internal-secret", config.zonesvr_internal_secret)`，与 ZoneSvr 服务端拦截器校验的 key 一致。若 GateSvr 封装了 ZoneRpcClient，可在该客户端内统一为每次调用注入 metadata，避免遗漏。

---

## 11. 阶段九：Qt 客户端开发

Qt5 桌面客户端通过 **WebSocket** 连接 GateSvr，使用 **Gate 的 ClientMessage/ServerMessage** 封装请求与响应，业务命令（如 `auth.login`、`chat.send_message`）由 Gate 转发至 ZoneSvr 的 HandleClientRequest，再按 cmd 分发到各后端服务。

### 11.1 项目结构

```
client/
├── CMakeLists.txt            # 根 CMake，包含 proto 与 desktop
├── proto/
│   └── CMakeLists.txt        # client_proto：从 backend 各服务 proto 生成 C++ 供客户端使用
│                             # 含 common、gate、auth、friend、chat、group、file
└── desktop/                  # SwiftChat 桌面应用
    ├── CMakeLists.txt
    ├── resources/
    │   └── resources.qrc
    └── src/
        ├── main.cpp              # 入口（QApplication，当前为占位）
        ├── ui/                    # UI 组件
        │   ├── loginwindow.*     # 登录窗口
        │   ├── mainwindow.*      # 主窗口
        │   ├── chatwidget.*      # 聊天区域
        │   ├── contactwidget.*   # 联系人/会话列表
        │   └── messageitem.*     # 单条消息展示
        ├── network/              # 网络层
        │   ├── websocket_client.*  # Qt WebSockets 封装
        │   └── protocol_handler.*  # 协议解析与 cmd 分发（request_id 匹配、推送通知）
        ├── models/               # 数据模型
        │   ├── user.*
        │   ├── message.*
        │   └── conversation.*
        └── utils/                # 工具类
            ├── settings.*       # 服务器 URL、登录信息持久化
            └── image_utils.*    # 图片加载/缓存
```

**说明：** 根项目 `CMakeLists.txt` 中 client 子目录默认被注释，需取消注释 `add_subdirectory(client)` 并安装 Qt5 后再构建客户端。

### 11.2 构建与运行

**依赖：** Qt5（Core、Gui、Widgets、Network、WebSockets），见 1.2 节。

```bash
# 1. 在项目根 CMakeLists.txt 中取消注释
#    add_subdirectory(client)

# 2. 配置与构建
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
         -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# 3. 运行桌面客户端（生成在 build/client/desktop/）
./client/desktop/SwiftChat
```

**Windows：** 使用 Qt5 的 CMake 配置（如 `Qt5_DIR`）并构建 Release，运行前可用 `windeployqt SwiftChat.exe` 收集 Qt 依赖。

### 11.3 客户端与网关协议

- **连接：** WebSocket，地址由配置/设置提供（如 `ws://host:9090/ws`），与 GateSvr 的 WebSocket 端口一致。
- **消息格式：** 见 `backend/gatesvr/proto/gate.proto`。
  - **上行（客户端 → Gate）：** `ClientMessage`：`cmd`（字符串）、`payload`（bytes）、`request_id`（字符串，用于匹配响应）。
  - **下行（Gate → 客户端）：** `ServerMessage`：`cmd`、`payload`、`request_id`、`code`（0 成功）、`message`（错误信息）。
- **请求流程：** Gate 收到 ClientMessage 后，按业务转发至 ZoneSvr 的 **HandleClientRequest**（gRPC），Zone 根据 `cmd` 前缀（auth/chat/friend/group/file）分发到对应 System，再调各后端 gRPC；响应经 Gate 原路返回为 ServerMessage。
- **常用 cmd 与 payload 约定（与 Zone HandleClientRequest 一致）：**

| cmd | 说明 | 请求 payload（序列化） | 响应 payload（code=0 时） |
|-----|------|------------------------|----------------------------|
| `auth.login` | 登录 | Zone: AuthLoginPayload（username, password, device_id, device_type） | AuthLoginResponsePayload（success, user_id, token, expire_at, error） |
| `auth.logout` | 登出 | AuthLogoutPayload（user_id, token） | 无 |
| `auth.validate_token` | 校验 Token | AuthValidateTokenPayload（token） | AuthValidateTokenResponsePayload（user_id） |
| `chat.send_message` | 发消息 | ChatSendMessagePayload（from_user_id, to_id, chat_type, content, …） | ChatSendMessageResponsePayload（success, msg_id, timestamp, error） |
| `chat.recall_message` | 撤回 | ChatRecallMessagePayload（msg_id, user_id） | 无 |
| `friend.add` / `friend.remove` / … | 好友 | 对应 Friend*Payload | 视接口 |
| `group.create` / `group.leave` / … | 群组 | 对应 Group*Payload | 视接口 |
| `file.get_upload_token` / `file.get_file_url` / … | 文件 | 对应 File*Payload | 对应 File*ResponsePayload |

- **服务端推送（cmd 为通知类型）：** 如 `chat.new_message`、`chat.recall`、`friend.request`、`user.status` 等，payload 为 gate.proto 中对应 Notify 消息的序列化（如 `NewMessageNotify`、`RecallNotify`）。客户端根据 `cmd` 解析 payload 并更新 UI。

以上 Payload 定义见 `backend/zonesvr/proto/zone.proto`（以及 gate.proto 中的 Notify）；客户端若引用 zone.proto 可复用同一结构，否则需与后端约定字段一致。

### 11.4 开发步骤

#### Step 1：WebSocket 连接

在 `network/websocket_client.cpp` 中连接 Gate 的 WebSocket 地址（来自 `Settings::serverUrl()`），连接成功后发出 `connected()`，收到二进制帧时发出 `messageReceived(QByteArray)`，发送时调用 `sendBinaryMessage`。与 Gate 的 WebSocket 服务端约定一致即可（通常为二进制 Protobuf）。

#### Step 2：协议处理（protocol_handler）

- **发送请求：** 使用 `swift::gate::ClientMessage`：设置 `cmd`、`payload`（bytes）、`request_id`（如 `req_1`、`req_2`），序列化后通过 `dataToSend` 交给 WebSocket 发送。
- **接收：** 解析为 `swift::gate::ServerMessage`。若 `request_id` 非空且在待处理表中，则视为某次请求的响应：调用对应 callback(code, payload) 并移除；否则按推送处理（如 `cmd == "chat.new_message"` 解析为 `NewMessageNotify` 并 emit `newMessageNotify`，同理 `chat.recall`、`friend.request` 等）。

这样与 gate.proto 及 Gate/Zone 的请求-响应、推送约定一致。

#### Step 3：登录界面（auth.login）

- 用户输入账号密码后，先建立 WebSocket 连接（Step 1），连接成功后再发登录。
- 构造 **auth.login** 请求：cmd 为 `"auth.login"`，payload 为 Zone 的 **AuthLoginPayload** 序列化（username、password、device_id、device_type；可与 zone.proto 或后端约定一致）。
- 在 `sendRequest` 的回调中：若 `code == 0`，解析 payload 为 **AuthLoginResponsePayload**，取 `user_id`、`token`、`expire_at`，写入 `Settings::saveLoginInfo`，再 emit `loginSuccess()` 进入主界面；否则提示 `message` 或“登录失败”。

#### Step 4：聊天界面（chat.send_message 与推送）

- **发送：** 构造 **chat.send_message**：cmd 为 `"chat.send_message"`，payload 为 **ChatSendMessagePayload**（from_user_id、to_id、chat_type、content、media_url/media_type 等），通过 `sendRequest` 发送；回调中若成功可将本地消息加入列表并清空输入框。
- **接收：** 在 ProtocolHandler 的推送分支中，对 `cmd == "chat.new_message"` 解析 `NewMessageNotify`，若 `chat_id` 为当前会话则更新消息列表（如 `addMessage(content, false, from_nickname)`）；对 `chat.recall` 解析 `RecallNotify` 并更新或移除对应消息项。

#### Step 5：主界面与联系人

- **MainWindow：** 左侧为联系人/会话列表（ContactWidget），右侧为当前会话的 ChatWidget；登录后从 Settings 读取 user_id/token，可选发 `friend.*` 拉取好友列表、`conversation` 相关 cmd 拉取会话列表（具体 cmd 与后端约定）。
- **设置：** 使用 `Settings::serverUrl()` 作为 WebSocket 地址，保证与 Gate 的 WebSocket 端口一致；Token 在后续需鉴权请求中可通过 Gate 与 Zone 的 metadata/约定带给后端（若 Gate 在转发时注入 token，则客户端只需在登录后保存并维持连接）。

### 11.5 打包发布

```bash
# Linux
cd build && make -j$(nproc)
# 可执行文件：client/desktop/SwiftChat；依赖 Qt5 运行库，需目标环境安装 Qt5 或打包时带上库。

# Windows（Release）
cmake --build . --config Release
windeployqt client/desktop/Release/SwiftChat.exe
# 将 windeployqt 生成的目录与 exe 一起分发；可选 NSIS/Inno Setup 制作安装包。

# macOS
# 使用 Qt 安装目录下的 macdeployqt 打包为 .app 并可选生成 dmg。
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

## 15. OnlineSvr 登录会话服务

OnlineSvr 独立管理**登录会话**与 **Token 签发**，ZoneSvr 的登录/登出直接走 OnlineSvr；AuthSvr 提供 VerifyCredentials（校验用户名密码并返回 user_id、profile），供 ZoneSvr 登录时先校验再调 OnlineSvr.Login。

### 15.1 职责

| 接口 | 说明 |
|------|------|
| **Login(user_id, device_id, device_type)** | 创建会话、签发 JWT，单设备策略（同设备幂等，异设备拒绝） |
| **Logout(user_id, token)** | 移除会话，使 Token 失效 |
| **ValidateToken(token)** | 校验 Token 并返回 user_id |

### 15.2 目录结构

```
backend/onlinesvr/
├── proto/online.proto           # Login / Logout / ValidateToken
├── cmd/main.cpp
├── CMakeLists.txt
├── Dockerfile
└── internal/
    ├── config/config.h/cpp
    ├── store/session_store.h/cpp   # RocksDB 会话存储
    ├── service/
    │   └── online_service.h/cpp   # 使用 common 的 swift::JwtCreate/JwtVerify（swift/jwt_helper.h）
    └── handler/online_handler.h/cpp
```

### 15.3 ZoneSvr 登录流程

1. **登录**：AuthSvr.VerifyCredentials(username, password) → user_id、profile；再 OnlineSvr.Login(user_id, device_id, device_type) → token。
2. **登出**：OnlineSvr.Logout(user_id, token)。
3. **Token 校验**：OnlineSvr.ValidateToken(token) → user_id。

AuthSystem 持有 AuthRpcClient 与 OnlineRpcClient，Login/Logout/ValidateToken 走 OnlineSvr。

### 15.4 端口与部署

- gRPC 端口：9095
- K8s：`deploy/k8s/onlinesvr-deployment.yaml`，已加入 kustomization

---

## 16. 业务服务请求身份校验（防伪造 user_id）

所有携带 `user_id` 的 gRPC 请求若不做校验，则存在越权：任意客户端可伪造 `user_id` 操作他人数据。因此 **AuthSvr（GetProfile/UpdateProfile）、FriendSvr、ChatSvr** 等需「当前用户」的接口统一从 **gRPC metadata 中的 JWT** 解析出当前用户，**不再信任请求体中的 user_id**。

### 16.1 约定

- **客户端**：在调用需鉴权的接口时，在 gRPC metadata 中携带 Token，任选其一：
  - `authorization: "Bearer <jwt>"`
  - `x-token: "<jwt>"`
- **服务端**：使用与 OnlineSvr 相同的 `jwt_secret` 校验 JWT，从 payload 得到 `user_id`，作为「当前用户」执行业务；未带 token 或 token 无效/过期则返回 `TOKEN_INVALID`（102）或 gRPC `UNAUTHENTICATED`。

### 16.2 各服务说明

| 服务 | 需 Token 的接口 | 不需 Token 的接口 |
|------|-----------------|-------------------|
| **AuthSvr** | GetProfile、UpdateProfile（仅能查/改本人资料） | Register、VerifyCredentials（登录前调用） |
| **FriendSvr** | 全部（好友列表、分组、黑名单等） | - |
| **ChatSvr** | 全部（消息、会话、群组相关） | - |

### 16.3 实现

- **公共库**：`backend/common` 提供 `swift_grpc_auth`，接口 `GetAuthenticatedUserId(ServerContext*, jwt_secret)`，从 metadata 读取 token 并调用 `JwtVerify`，成功返回 `user_id`，否则返回空串。
- **配置**：AuthSvr、FriendSvr、ChatSvr 的 config 均增加 `jwt_secret`（默认与 OnlineSvr 一致，如 `swift_online_secret_2026`），可通过配置文件或环境变量 `AUTHSVR_JWT_SECRET` / `FRIENDSVR_JWT_SECRET` / `CHATSVR_JWT_SECRET` 覆盖。
- **Handler**：需鉴权的 RPC 入口先调用 `GetAuthenticatedUserId`，得到 `uid`；若为空则写响应 `code=TOKEN_INVALID`（或 gRPC Status `UNAUTHENTICATED`）并 return；否则用 `uid` 作为「当前用户」调用 Service（不再使用请求体中的 `user_id` / `from_user_id` 等）。

这样，即使用户 A 登录后伪造请求体中的 `user_id=B`，服务端也会以 token 中的 A 作为操作主体，无法越权操作 B 的数据。

---

**文档版本**：v1.0  
**最后更新**：2026年2月2日
