#pragma once

#include <string>
#include <optional>
#include <memory>

namespace swift::online {

struct SessionData {
    std::string user_id;
    std::string device_id;
    std::string token;
    int64_t login_time = 0;
    int64_t expire_at = 0;
};

class SessionStore {
public:
    virtual ~SessionStore() = default;

    virtual bool SetSession(const SessionData& session) = 0;
    virtual std::optional<SessionData> GetSession(const std::string& user_id) = 0;
    virtual bool RemoveSession(const std::string& user_id) = 0;
};

class RocksDBSessionStore : public SessionStore {
public:
    explicit RocksDBSessionStore(const std::string& db_path);
    ~RocksDBSessionStore() override;

    bool SetSession(const SessionData& session) override;
    std::optional<SessionData> GetSession(const std::string& user_id) override;
    bool RemoveSession(const std::string& user_id) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace swift::online
