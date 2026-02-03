/**
 * @file friend_store_test.cpp
 * @brief FriendStore 单元测试
 */

#include "friend_store.h"
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>

namespace swift::friend_ {

class FriendStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ =
            "/tmp/friendstore_test_" +
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        store_ = std::make_unique<RocksDBFriendStore>(test_db_path_);
    }

    void TearDown() override {
        store_.reset();
        std::filesystem::remove_all(test_db_path_);
    }

    FriendData MakeFriend(const std::string& uid, const std::string& fid,
                          const std::string& group = "", int64_t at = 1000) {
        FriendData d;
        d.user_id = uid;
        d.friend_id = fid;
        d.remark = "remark_" + fid;
        d.group_id = group;
        d.added_at = at;
        return d;
    }

    FriendRequestData MakeRequest(const std::string& req_id, const std::string& from,
                                   const std::string& to, int status = 0, int64_t at = 2000) {
        FriendRequestData r;
        r.request_id = req_id;
        r.from_user_id = from;
        r.to_user_id = to;
        r.remark = "hello";
        r.status = status;
        r.created_at = at;
        return r;
    }

    FriendGroupData MakeGroup(const std::string& uid, const std::string& gid,
                               const std::string& name, int order = 0) {
        FriendGroupData g;
        g.group_id = gid;
        g.user_id = uid;
        g.group_name = name;
        g.sort_order = order;
        return g;
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBFriendStore> store_;
};

// ============================================================================
// 好友关系
// ============================================================================

TEST_F(FriendStoreTest, AddFriend_Success) {
    FriendData d = MakeFriend("u1", "u2", "g1");
    EXPECT_TRUE(store_->AddFriend(d));
    EXPECT_TRUE(store_->IsFriend("u1", "u2"));
    EXPECT_TRUE(store_->IsFriend("u2", "u1"));
}

TEST_F(FriendStoreTest, GetFriends_EmptyGroupId_Excluded) {
    store_->AddFriend(MakeFriend("u1", "u2", ""));  // 非法：空分组
    auto list = store_->GetFriends("u1");
    EXPECT_EQ(list.size(), 0u);
}

TEST_F(FriendStoreTest, AddFriend_Duplicate_Fail) {
    FriendData d = MakeFriend("u1", "u2");
    EXPECT_TRUE(store_->AddFriend(d));
    EXPECT_FALSE(store_->AddFriend(d));
}

TEST_F(FriendStoreTest, RemoveFriend_Success) {
    store_->AddFriend(MakeFriend("u1", "u2"));
    EXPECT_TRUE(store_->RemoveFriend("u1", "u2"));
    EXPECT_FALSE(store_->IsFriend("u1", "u2"));
    EXPECT_FALSE(store_->IsFriend("u2", "u1"));
}

TEST_F(FriendStoreTest, GetFriends_All) {
    store_->AddFriend(MakeFriend("u1", "u2", kDefaultFriendGroupId, 1));
    store_->AddFriend(MakeFriend("u1", "u3", kDefaultFriendGroupId, 2));
    store_->AddFriend(MakeFriend("u1", "u4", "g1", 3));

    auto list = store_->GetFriends("u1");
    EXPECT_EQ(list.size(), 3u);
}

TEST_F(FriendStoreTest, GetFriends_ByGroup) {
    store_->AddFriend(MakeFriend("u1", "u2", "g1"));
    store_->AddFriend(MakeFriend("u1", "u3", "g1"));
    store_->AddFriend(MakeFriend("u1", "u4", "g2"));

    auto list = store_->GetFriends("u1", "g1");
    EXPECT_EQ(list.size(), 2u);
    for (const auto& f : list) {
        EXPECT_EQ(f.group_id, "g1");
    }
}

TEST_F(FriendStoreTest, UpdateRemark_Success) {
    store_->AddFriend(MakeFriend("u1", "u2", kDefaultFriendGroupId));
    EXPECT_TRUE(store_->UpdateRemark("u1", "u2", "new_remark"));

    auto list = store_->GetFriends("u1");
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].remark, "new_remark");
}

TEST_F(FriendStoreTest, MoveFriend_Success) {
    store_->AddFriend(MakeFriend("u1", "u2", "g1"));
    EXPECT_TRUE(store_->MoveFriend("u1", "u2", "g2"));

    auto list = store_->GetFriends("u1", "g2");
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].friend_id, "u2");
}

// ============================================================================
// 好友请求
// ============================================================================

TEST_F(FriendStoreTest, CreateRequest_Success) {
    FriendRequestData r = MakeRequest("req1", "u1", "u2");
    EXPECT_TRUE(store_->CreateRequest(r));

    auto got = store_->GetRequest("req1");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->from_user_id, "u1");
    EXPECT_EQ(got->to_user_id, "u2");
    EXPECT_EQ(got->status, 0);
}

TEST_F(FriendStoreTest, CreateRequest_Duplicate_Fail) {
    FriendRequestData r = MakeRequest("req1", "u1", "u2");
    EXPECT_TRUE(store_->CreateRequest(r));
    EXPECT_FALSE(store_->CreateRequest(r));
}

TEST_F(FriendStoreTest, GetReceivedRequests) {
    store_->CreateRequest(MakeRequest("r1", "u2", "u1"));
    store_->CreateRequest(MakeRequest("r2", "u3", "u1"));

    auto list = store_->GetReceivedRequests("u1");
    EXPECT_EQ(list.size(), 2u);
}

TEST_F(FriendStoreTest, GetSentRequests) {
    store_->CreateRequest(MakeRequest("r1", "u1", "u2"));
    store_->CreateRequest(MakeRequest("r2", "u1", "u3"));

    auto list = store_->GetSentRequests("u1");
    EXPECT_EQ(list.size(), 2u);
}

TEST_F(FriendStoreTest, UpdateRequestStatus_Success) {
    store_->CreateRequest(MakeRequest("req1", "u1", "u2", 0));
    EXPECT_TRUE(store_->UpdateRequestStatus("req1", 1));

    auto got = store_->GetRequest("req1");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->status, 1);
}

// ============================================================================
// 分组
// ============================================================================

TEST_F(FriendStoreTest, CreateGroup_Success) {
    FriendGroupData g = MakeGroup("u1", "g1", "好友", 0);
    EXPECT_TRUE(store_->CreateGroup(g));

    auto list = store_->GetGroups("u1");
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].group_id, "g1");
    EXPECT_EQ(list[0].group_name, "好友");
}

TEST_F(FriendStoreTest, CreateGroup_Duplicate_Fail) {
    store_->CreateGroup(MakeGroup("u1", "g1", "A"));
    EXPECT_FALSE(store_->CreateGroup(MakeGroup("u1", "g1", "B")));
}

TEST_F(FriendStoreTest, DeleteGroup_MovesFriendsToDefault) {
    store_->CreateGroup(MakeGroup("u1", "g1", "组1"));
    store_->AddFriend(MakeFriend("u1", "u2", "g1"));
    EXPECT_TRUE(store_->DeleteGroup("u1", "g1"));

    EXPECT_EQ(store_->GetGroups("u1").size(), 0u);
    auto friends = store_->GetFriends("u1");
    ASSERT_EQ(friends.size(), 1u);
    EXPECT_EQ(friends[0].group_id, kDefaultFriendGroupId);
}

// ============================================================================
// 黑名单
// ============================================================================

TEST_F(FriendStoreTest, Block_Unblock) {
    EXPECT_TRUE(store_->Block("u1", "u2"));
    EXPECT_TRUE(store_->IsBlocked("u1", "u2"));

    EXPECT_TRUE(store_->Unblock("u1", "u2"));
    EXPECT_FALSE(store_->IsBlocked("u1", "u2"));
}

TEST_F(FriendStoreTest, GetBlockList) {
    store_->Block("u1", "u2");
    store_->Block("u1", "u3");

    auto list = store_->GetBlockList("u1");
    EXPECT_EQ(list.size(), 2u);
    EXPECT_TRUE(std::find(list.begin(), list.end(), "u2") != list.end());
    EXPECT_TRUE(std::find(list.begin(), list.end(), "u3") != list.end());
}

// ============================================================================
// 综合
// ============================================================================

TEST_F(FriendStoreTest, PersistenceAcrossReopen) {
    store_->AddFriend(MakeFriend("u1", "u2", kDefaultFriendGroupId));
    store_->CreateRequest(MakeRequest("req1", "u1", "u2"));
    store_->CreateGroup(MakeGroup("u1", "g1", "默认"));
    store_->Block("u1", "u3");

    store_.reset();
    store_ = std::make_unique<RocksDBFriendStore>(test_db_path_);

    EXPECT_TRUE(store_->IsFriend("u1", "u2"));
    auto friends = store_->GetFriends("u1");
    ASSERT_EQ(friends.size(), 1u);
    EXPECT_EQ(friends[0].group_id, kDefaultFriendGroupId);
    ASSERT_TRUE(store_->GetRequest("req1").has_value());
    EXPECT_EQ(store_->GetGroups("u1").size(), 1u);
    EXPECT_TRUE(store_->IsBlocked("u1", "u3"));
}

}  // namespace swift::friend_

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
