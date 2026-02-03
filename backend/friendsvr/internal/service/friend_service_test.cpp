/**
 * @file friend_service_test.cpp
 * @brief FriendService 业务逻辑单元测试
 */

#include "friend_service.h"
#include "../store/friend_store.h"
#include "swift/error_code.h"
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>

namespace swift::friend_ {

class FriendServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        path_ = "/tmp/friend_service_test_" +
               std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        store_ = std::make_shared<RocksDBFriendStore>(path_);
        service_ = std::make_unique<FriendService>(store_);
    }
    void TearDown() override {
        service_.reset();
        store_.reset();
        std::filesystem::remove_all(path_);
    }
    std::string path_;
    std::shared_ptr<RocksDBFriendStore> store_;
    std::unique_ptr<FriendService> service_;
};

TEST_F(FriendServiceTest, AddFriend_SendRequest) {
    EXPECT_TRUE(service_->AddFriend("u1", "u2", "hello"));
    auto received = service_->GetFriendRequests("u2", 1);
    ASSERT_EQ(received.size(), 1u);
    EXPECT_EQ(received[0].from_user_id, "u1");
    EXPECT_EQ(received[0].to_user_id, "u2");
    EXPECT_EQ(received[0].remark, "hello");
    EXPECT_EQ(received[0].status, 0);
}

TEST_F(FriendServiceTest, AddFriend_Self_Fail) {
    EXPECT_FALSE(service_->AddFriend("u1", "u1", ""));
}

TEST_F(FriendServiceTest, AddFriend_AlreadyFriends_Fail) {
    store_->AddFriend(FriendData{"u1", "u2", "", kDefaultFriendGroupId, 1000});
    EXPECT_FALSE(service_->AddFriend("u1", "u2", ""));
}

TEST_F(FriendServiceTest, HandleRequest_Accept_BecomesFriend) {
    EXPECT_TRUE(service_->AddFriend("u1", "u2", "hi"));
    auto received = service_->GetFriendRequests("u2", 1);
    ASSERT_EQ(received.size(), 1u);
    std::string req_id = received[0].request_id;

    EXPECT_TRUE(service_->HandleRequest("u2", req_id, true, "g1"));

    auto friends = service_->GetFriends("u2");
    ASSERT_EQ(friends.size(), 1u);
    EXPECT_EQ(friends[0].friend_id, "u1");
    EXPECT_EQ(friends[0].group_id, "g1");
    EXPECT_TRUE(store_->IsFriend("u1", "u2"));
}

TEST_F(FriendServiceTest, HandleRequest_Reject) {
    EXPECT_TRUE(service_->AddFriend("u1", "u2", ""));
    auto received = service_->GetFriendRequests("u2", 1);
    std::string req_id = received[0].request_id;
    EXPECT_TRUE(service_->HandleRequest("u2", req_id, false, ""));
    EXPECT_EQ(service_->GetFriends("u2").size(), 0u);
    auto req = store_->GetRequest(req_id);
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->status, 2);
}

TEST_F(FriendServiceTest, RemoveFriend) {
    store_->AddFriend(FriendData{"u1", "u2", "", kDefaultFriendGroupId, 1000});
    EXPECT_TRUE(service_->RemoveFriend("u1", "u2"));
    EXPECT_FALSE(store_->IsFriend("u1", "u2"));
}

TEST_F(FriendServiceTest, Block_Unblock) {
    EXPECT_TRUE(service_->Block("u1", "u2"));
    auto list = service_->GetBlockList("u1");
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0], "u2");
    EXPECT_TRUE(service_->Unblock("u1", "u2"));
    EXPECT_TRUE(service_->GetBlockList("u1").empty());
}

TEST_F(FriendServiceTest, CreateFriendGroup) {
    std::string gid;
    EXPECT_TRUE(service_->CreateFriendGroup("u1", "同事", &gid));
    EXPECT_FALSE(gid.empty());
    auto groups = service_->GetFriendGroups("u1");
    ASSERT_GE(groups.size(), 1u);
    bool has_colleague = false;
    for (const auto& g : groups)
        if (g.group_name == "同事") { has_colleague = true; break; }
    EXPECT_TRUE(has_colleague);
}

TEST_F(FriendServiceTest, SetRemark) {
    store_->AddFriend(FriendData{"u1", "u2", "old", kDefaultFriendGroupId, 1000});
    EXPECT_TRUE(service_->SetRemark("u1", "u2", "new_remark"));
    auto friends = service_->GetFriends("u1");
    ASSERT_EQ(friends.size(), 1u);
    EXPECT_EQ(friends[0].remark, "new_remark");
}

TEST_F(FriendServiceTest, GetFriendGroups_ReturnsDefaultGroup) {
    auto groups = service_->GetFriendGroups("u1");
    ASSERT_GE(groups.size(), 1u);
    bool has_default = false;
    for (const auto& g : groups)
        if (g.group_id == kDefaultFriendGroupId && g.group_name == kDefaultFriendGroupName) {
            has_default = true;
            break;
        }
    EXPECT_TRUE(has_default);
}

TEST_F(FriendServiceTest, HandleRequest_Accept_EmptyGroup_UsesDefault) {
    EXPECT_TRUE(service_->AddFriend("u1", "u2", "hi"));
    auto received = service_->GetFriendRequests("u2", 1);
    ASSERT_EQ(received.size(), 1u);
    std::string req_id = received[0].request_id;
    EXPECT_TRUE(service_->HandleRequest("u2", req_id, true, ""));
    auto friends = service_->GetFriends("u2");
    ASSERT_EQ(friends.size(), 1u);
    EXPECT_EQ(friends[0].group_id, kDefaultFriendGroupId);
}

TEST_F(FriendServiceTest, DeleteFriendGroup_Default_Fail) {
    service_->GetFriendGroups("u1");
    swift::ErrorCode ec = service_->DeleteFriendGroup("u1", kDefaultFriendGroupId);
    EXPECT_EQ(ec, swift::ErrorCode::FRIEND_GROUP_DEFAULT);
}

TEST_F(FriendServiceTest, DeleteFriendGroup_EmptyId_Fail) {
    swift::ErrorCode ec = service_->DeleteFriendGroup("u1", "");
    EXPECT_EQ(ec, swift::ErrorCode::FRIEND_GROUP_DEFAULT);
}

TEST_F(FriendServiceTest, DeleteFriendGroup_NotFound_Fail) {
    swift::ErrorCode ec = service_->DeleteFriendGroup("u1", "nonexistent_gid");
    EXPECT_EQ(ec, swift::ErrorCode::FRIEND_GROUP_NOT_FOUND);
}

TEST_F(FriendServiceTest, DeleteFriendGroup_Success_MovesToDefault) {
    std::string gid;
    service_->CreateFriendGroup("u1", "自定义", &gid);
    service_->GetFriendGroups("u1");
    store_->AddFriend(FriendData{"u1", "u2", "", gid, 1000});
    swift::ErrorCode ec = service_->DeleteFriendGroup("u1", gid);
    EXPECT_EQ(ec, swift::ErrorCode::OK);
    auto friends = service_->GetFriends("u1");
    ASSERT_EQ(friends.size(), 1u);
    EXPECT_EQ(friends[0].group_id, kDefaultFriendGroupId);
}

}  // namespace swift::friend_

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
