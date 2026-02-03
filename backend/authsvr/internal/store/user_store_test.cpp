/**
 * @file user_store_test.cpp
 * @brief UserStore 单元测试
 */

#include "user_store.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>

namespace swift::auth {

class UserStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_db_path_ = "/tmp/userstore_test_" + 
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        store_ = std::make_unique<RocksDBUserStore>(test_db_path_);
    }

    void TearDown() override {
        // 关闭数据库
        store_.reset();
        // 删除测试目录
        std::filesystem::remove_all(test_db_path_);
    }

    // 创建测试用户数据
    UserData CreateTestUser(const std::string& suffix = "") {
        UserData user;
        user.user_id = "uid_test" + suffix;
        user.username = "testuser" + suffix;
        user.password_hash = "hash_abc123" + suffix;
        user.nickname = "Test User" + suffix;
        user.avatar_url = "https://example.com/avatar" + suffix + ".png";
        user.signature = "Hello World" + suffix;
        user.gender = 1;
        user.created_at = 1700000000;
        user.updated_at = 1700000000;
        return user;
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBUserStore> store_;
};

// ============================================================================
// Create 测试
// ============================================================================

TEST_F(UserStoreTest, CreateUser_Success) {
    UserData user = CreateTestUser();
    
    EXPECT_TRUE(store_->Create(user));
}

TEST_F(UserStoreTest, CreateUser_DuplicateUsername_Fail) {
    UserData user1 = CreateTestUser("1");
    UserData user2 = CreateTestUser("2");
    user2.username = user1.username;  // 相同用户名
    
    EXPECT_TRUE(store_->Create(user1));
    EXPECT_FALSE(store_->Create(user2));  // 应该失败
}

TEST_F(UserStoreTest, CreateUser_DuplicateUserId_Success) {
    // 注意：当前实现不检查 user_id 重复，只检查 username
    UserData user1 = CreateTestUser("1");
    UserData user2 = CreateTestUser("2");
    user2.user_id = user1.user_id;  // 相同 user_id
    
    EXPECT_TRUE(store_->Create(user1));
    // 第二次会覆盖第一次的数据（这是当前设计，可根据需求调整）
    EXPECT_TRUE(store_->Create(user2));
}

// ============================================================================
// GetById 测试
// ============================================================================

TEST_F(UserStoreTest, GetById_Exists) {
    UserData user = CreateTestUser();
    store_->Create(user);
    
    auto result = store_->GetById(user.user_id);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->user_id, user.user_id);
    EXPECT_EQ(result->username, user.username);
    EXPECT_EQ(result->password_hash, user.password_hash);
    EXPECT_EQ(result->nickname, user.nickname);
    EXPECT_EQ(result->avatar_url, user.avatar_url);
    EXPECT_EQ(result->signature, user.signature);
    EXPECT_EQ(result->gender, user.gender);
    EXPECT_EQ(result->created_at, user.created_at);
    EXPECT_EQ(result->updated_at, user.updated_at);
}

TEST_F(UserStoreTest, GetById_NotExists) {
    auto result = store_->GetById("nonexistent_user_id");
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(UserStoreTest, GetById_EmptyId) {
    auto result = store_->GetById("");
    
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// GetByUsername 测试
// ============================================================================

TEST_F(UserStoreTest, GetByUsername_Exists) {
    UserData user = CreateTestUser();
    store_->Create(user);
    
    auto result = store_->GetByUsername(user.username);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->user_id, user.user_id);
    EXPECT_EQ(result->username, user.username);
}

TEST_F(UserStoreTest, GetByUsername_NotExists) {
    auto result = store_->GetByUsername("nonexistent_username");
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(UserStoreTest, GetByUsername_EmptyUsername) {
    auto result = store_->GetByUsername("");
    
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Update 测试
// ============================================================================

TEST_F(UserStoreTest, Update_Success) {
    UserData user = CreateTestUser();
    store_->Create(user);
    
    // 修改部分字段
    user.nickname = "Updated Nickname";
    user.avatar_url = "https://example.com/new_avatar.png";
    user.signature = "New signature";
    user.updated_at = 1700000100;
    
    EXPECT_TRUE(store_->Update(user));
    
    // 验证更新
    auto result = store_->GetById(user.user_id);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->nickname, "Updated Nickname");
    EXPECT_EQ(result->avatar_url, "https://example.com/new_avatar.png");
    EXPECT_EQ(result->signature, "New signature");
    EXPECT_EQ(result->updated_at, 1700000100);
}

TEST_F(UserStoreTest, Update_ChangeUsername_Success) {
    UserData user = CreateTestUser();
    store_->Create(user);
    
    std::string old_username = user.username;
    user.username = "new_username";
    
    EXPECT_TRUE(store_->Update(user));
    
    // 验证旧用户名不存在
    EXPECT_FALSE(store_->UsernameExists(old_username));
    // 验证新用户名存在
    EXPECT_TRUE(store_->UsernameExists(user.username));
    // 验证通过新用户名能查到
    auto result = store_->GetByUsername(user.username);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->user_id, user.user_id);
}

TEST_F(UserStoreTest, Update_ChangeUsername_Conflict) {
    UserData user1 = CreateTestUser("1");
    UserData user2 = CreateTestUser("2");
    store_->Create(user1);
    store_->Create(user2);
    
    // 尝试将 user1 的用户名改成 user2 的
    user1.username = user2.username;
    
    EXPECT_FALSE(store_->Update(user1));  // 应该失败
}

TEST_F(UserStoreTest, Update_NotExists) {
    UserData user = CreateTestUser();
    // 不创建，直接更新
    
    EXPECT_FALSE(store_->Update(user));
}

TEST_F(UserStoreTest, Update_EmptyUserId) {
    UserData user = CreateTestUser();
    user.user_id = "";
    
    EXPECT_FALSE(store_->Update(user));
}

// ============================================================================
// UsernameExists 测试
// ============================================================================

TEST_F(UserStoreTest, UsernameExists_True) {
    UserData user = CreateTestUser();
    store_->Create(user);
    
    EXPECT_TRUE(store_->UsernameExists(user.username));
}

TEST_F(UserStoreTest, UsernameExists_False) {
    EXPECT_FALSE(store_->UsernameExists("nonexistent_username"));
}

TEST_F(UserStoreTest, UsernameExists_Empty) {
    EXPECT_FALSE(store_->UsernameExists(""));
}

// ============================================================================
// 综合测试
// ============================================================================

TEST_F(UserStoreTest, MultipleUsers) {
    // 创建多个用户
    for (int i = 0; i < 10; ++i) {
        UserData user = CreateTestUser(std::to_string(i));
        EXPECT_TRUE(store_->Create(user));
    }
    
    // 验证所有用户都能查到
    for (int i = 0; i < 10; ++i) {
        std::string suffix = std::to_string(i);
        auto result = store_->GetById("uid_test" + suffix);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->username, "testuser" + suffix);
    }
}

TEST_F(UserStoreTest, PersistenceAcrossReopen) {
    UserData user = CreateTestUser();
    store_->Create(user);
    
    // 关闭数据库
    store_.reset();
    
    // 重新打开
    store_ = std::make_unique<RocksDBUserStore>(test_db_path_);
    
    // 验证数据仍然存在
    auto result = store_->GetById(user.user_id);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->username, user.username);
}

}  // namespace swift::auth

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
