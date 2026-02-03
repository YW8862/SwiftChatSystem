/**
 * @file auth_service_test.cpp
 * @brief AuthService 单元测试
 */

#include "auth_service.h"
#include "../store/user_store.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>

namespace swift::auth {

class AuthServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto suffix = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        user_db_path_ = "/tmp/auth_service_user_" + suffix;
        store_ = std::make_shared<RocksDBUserStore>(user_db_path_);
        service_ = std::make_unique<AuthService>(store_);
    }

    void TearDown() override {
        service_.reset();
        store_.reset();
        std::filesystem::remove_all(user_db_path_);
    }

    std::string user_db_path_;
    std::shared_ptr<RocksDBUserStore> store_;
    std::unique_ptr<AuthService> service_;
};

// ============================================================================
// Register
// ============================================================================

TEST_F(AuthServiceTest, Register_Success) {
    auto result = service_->Register("alice", "password123", "Alice", "alice@example.com");
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.user_id.empty());
    EXPECT_TRUE(result.user_id.substr(0, 2) == "u_");
}

TEST_F(AuthServiceTest, Register_ShortUsername_Fail) {
    auto result = service_->Register("ab", "password123", "AB", "");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}

TEST_F(AuthServiceTest, Register_ShortPassword_Fail) {
    auto result = service_->Register("alice", "12345", "Alice", "");
    EXPECT_FALSE(result.success);
}

TEST_F(AuthServiceTest, Register_DuplicateUsername_Fail) {
    service_->Register("alice", "password123", "Alice", "");
    auto result = service_->Register("alice", "password456", "Alice2", "");
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error.find("already exists") != std::string::npos);
}

TEST_F(AuthServiceTest, Register_WithAvatar) {
    auto result = service_->Register("alice", "password123", "Alice", "", "https://example.com/avatar.png");
    EXPECT_TRUE(result.success);
    auto profile = service_->GetProfile(result.user_id);
    ASSERT_TRUE(profile.has_value());
    EXPECT_EQ(profile->avatar_url, "https://example.com/avatar.png");
}

// ============================================================================
// VerifyCredentials
// ============================================================================

TEST_F(AuthServiceTest, VerifyCredentials_Success) {
    auto reg = service_->Register("alice", "password123", "Alice", "alice@example.com");
    ASSERT_TRUE(reg.success);

    auto result = service_->VerifyCredentials("alice", "password123");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.user_id, reg.user_id);
    ASSERT_TRUE(result.profile.has_value());
    EXPECT_EQ(result.profile->username, "alice");
    EXPECT_EQ(result.profile->nickname, "Alice");
}

TEST_F(AuthServiceTest, VerifyCredentials_WrongPassword) {
    service_->Register("alice", "password123", "Alice", "");
    auto result = service_->VerifyCredentials("alice", "wrongpassword");
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error.find("Wrong password") != std::string::npos);
}

TEST_F(AuthServiceTest, VerifyCredentials_UserNotFound) {
    auto result = service_->VerifyCredentials("nonexistent", "password123");
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error.find("User not found") != std::string::npos);
}

// ============================================================================
// GetProfile
// ============================================================================

TEST_F(AuthServiceTest, GetProfile_Exists) {
    auto reg = service_->Register("alice", "password123", "Alice", "alice@example.com");
    ASSERT_TRUE(reg.success);

    auto profile = service_->GetProfile(reg.user_id);
    ASSERT_TRUE(profile.has_value());
    EXPECT_EQ(profile->user_id, reg.user_id);
    EXPECT_EQ(profile->username, "alice");
    EXPECT_EQ(profile->nickname, "Alice");
}

TEST_F(AuthServiceTest, GetProfile_NotExists) {
    auto profile = service_->GetProfile("nonexistent_user_id");
    EXPECT_FALSE(profile.has_value());
}

// ============================================================================
// UpdateProfile
// ============================================================================

TEST_F(AuthServiceTest, UpdateProfile_Success) {
    auto reg = service_->Register("alice", "password123", "Alice", "");
    ASSERT_TRUE(reg.success);

    auto result = service_->UpdateProfile(reg.user_id, "Alice Updated", "https://avatar.png", "Hello");
    EXPECT_TRUE(result.success);

    auto profile = service_->GetProfile(reg.user_id);
    ASSERT_TRUE(profile.has_value());
    EXPECT_EQ(profile->nickname, "Alice Updated");
    EXPECT_EQ(profile->avatar_url, "https://avatar.png");
    EXPECT_EQ(profile->signature, "Hello");
}

TEST_F(AuthServiceTest, UpdateProfile_Partial) {
    auto reg = service_->Register("alice", "password123", "Alice", "", "old_avatar.png");
    ASSERT_TRUE(reg.success);

    // 只更新 nickname
    auto result = service_->UpdateProfile(reg.user_id, "Alice New", "", "");
    EXPECT_TRUE(result.success);

    auto profile = service_->GetProfile(reg.user_id);
    ASSERT_TRUE(profile.has_value());
    EXPECT_EQ(profile->nickname, "Alice New");
    EXPECT_EQ(profile->avatar_url, "old_avatar.png");  // 未更新
}

TEST_F(AuthServiceTest, UpdateProfile_NotExists) {
    auto result = service_->UpdateProfile("nonexistent", "Nick", "", "");
    EXPECT_FALSE(result.success);
}

// ============================================================================
// 综合
// ============================================================================

TEST_F(AuthServiceTest, FullFlow) {
    auto reg = service_->Register("bob", "bob123456", "Bob", "bob@test.com");
    ASSERT_TRUE(reg.success);

    auto verify = service_->VerifyCredentials("bob", "bob123456");
    ASSERT_TRUE(verify.success);
    EXPECT_EQ(verify.user_id, reg.user_id);

    auto update = service_->UpdateProfile(reg.user_id, "Bob Updated", "", "Hi there");
    EXPECT_TRUE(update.success);

    auto profile = service_->GetProfile(reg.user_id);
    ASSERT_TRUE(profile.has_value());
    EXPECT_EQ(profile->nickname, "Bob Updated");
    EXPECT_EQ(profile->signature, "Hi there");
}

}  // namespace swift::auth

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
