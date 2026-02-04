/**
 * @file chat_service_test.cpp
 * @brief ChatServiceCore 单元测试：私聊、群聊、撤回、已读、会话删除等。
 */

#include "../store/message_store.h"
#include "../store/group_store.h"
#include "chat_service.h"
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>

namespace swift::chat {

class ChatServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto suffix = std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
        msg_db_path_ = "/tmp/chat_msg_" + suffix;
        conv_db_path_ = "/tmp/chat_conv_" + suffix;
        conv_meta_db_path_ = "/tmp/chat_conv_meta_" + suffix;
        group_db_path_ = "/tmp/chat_group_" + suffix;

        msg_store_ = std::make_shared<RocksDBMessageStore>(msg_db_path_);
        conv_store_ = std::make_shared<RocksDBConversationStore>(conv_db_path_);
        conv_registry_ = std::make_shared<RocksDBConversationRegistry>(conv_meta_db_path_);
        group_store_ = std::make_shared<swift::group_::RocksDBGroupStore>(group_db_path_);

        service_ = std::make_unique<ChatServiceCore>(
            msg_store_, conv_store_, conv_registry_, group_store_);
    }

    void TearDown() override {
        service_.reset();
        group_store_.reset();
        conv_registry_.reset();
        conv_store_.reset();
        msg_store_.reset();
        std::filesystem::remove_all(msg_db_path_);
        std::filesystem::remove_all(conv_db_path_);
        std::filesystem::remove_all(conv_meta_db_path_);
        std::filesystem::remove_all(group_db_path_);
    }

    std::string msg_db_path_;
    std::string conv_db_path_;
    std::string conv_meta_db_path_;
    std::string group_db_path_;

    std::shared_ptr<RocksDBMessageStore> msg_store_;
    std::shared_ptr<RocksDBConversationStore> conv_store_;
    std::shared_ptr<RocksDBConversationRegistry> conv_registry_;
    std::shared_ptr<swift::group_::RocksDBGroupStore> group_store_;
    std::unique_ptr<ChatServiceCore> service_;
};

// 私聊发送消息：应成功写入消息、会话、离线队列
TEST_F(ChatServiceTest, SendMessage_Private_Success) {
    auto result = service_->SendMessage(
        "u1", "u2", ChatType::PRIVATE,
        "hello", "", "", {}, "");

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.msg_id.empty());
    EXPECT_FALSE(result.conversation_id.empty());

    // 消息存在
    auto msg = msg_store_->GetById(result.msg_id);
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->from_user_id, "u1");
    EXPECT_EQ(msg->to_id, "u2");
    EXPECT_EQ(msg->conversation_id, result.conversation_id);

    // 会话：两端各一条
    auto convs_u1 = conv_store_->GetList("u1");
    auto convs_u2 = conv_store_->GetList("u2");
    ASSERT_EQ(convs_u1.size(), 1u);
    ASSERT_EQ(convs_u2.size(), 1u);
    EXPECT_EQ(convs_u1[0].conversation_id, result.conversation_id);
    EXPECT_EQ(convs_u2[0].conversation_id, result.conversation_id);

    // 离线：u2 有一条
    std::string cursor;
    bool has_more = false;
    auto offline = msg_store_->PullOffline("u2", "", 10, cursor, has_more);
    ASSERT_EQ(offline.size(), 1u);
    EXPECT_EQ(offline[0].msg_id, result.msg_id);
}

// 撤回消息：发送者 2 分钟内允许撤回，状态更新
TEST_F(ChatServiceTest, RecallMessage_Success) {
    auto send = service_->SendMessage(
        "u1", "u2", ChatType::PRIVATE,
        "hello recall", "", "", {}, "");
    ASSERT_TRUE(send.success);

    auto result = service_->RecallMessage(send.msg_id, "u1");
    EXPECT_TRUE(result.success);

    auto msg = msg_store_->GetById(send.msg_id);
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->status, 1);
    EXPECT_GT(msg->recall_at, 0);
}

// 私聊删除会话：应返回 CONVERSATION_PRIVATE_CANNOT_DELETE
TEST_F(ChatServiceTest, DeleteConversation_Private_ReturnsError) {
    auto send = service_->SendMessage(
        "u1", "u2", ChatType::PRIVATE,
        "hello", "", "", {}, "");
    ASSERT_TRUE(send.success);

    auto res = service_->DeleteConversation("u1", "u2", ChatType::PRIVATE);
    EXPECT_FALSE(res.success);
    EXPECT_EQ(res.error, swift::ErrorCode::CONVERSATION_PRIVATE_CANNOT_DELETE);
}

// 群聊删除会话：仅群主可解散，解散后会话从所有成员列表移除，历史为空
TEST_F(ChatServiceTest, DeleteConversation_Group_OwnerDissolve) {
    // 准备群组与成员
    swift::group_::GroupData g;
    g.group_id = "g1";
    g.group_name = "TestGroup";
    g.owner_id = "owner";
    ASSERT_TRUE(group_store_->CreateGroup(g));

    swift::group_::GroupMemberData m_owner;
    m_owner.user_id = "owner";
    swift::group_::GroupMemberData m_member;
    m_member.user_id = "member";
    ASSERT_TRUE(group_store_->AddMember("g1", m_owner));
    ASSERT_TRUE(group_store_->AddMember("g1", m_member));

    // 群聊发一条消息，确保产生会话
    auto send = service_->SendMessage(
        "owner", "g1", ChatType::GROUP,
        "group msg", "", "", {}, "");
    ASSERT_TRUE(send.success);

    // 删除会话（解散群）
    auto res = service_->DeleteConversation("owner", "g1", ChatType::GROUP);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.error, swift::ErrorCode::OK);

    // 会话应从所有成员会话列表中移除
    auto convs_owner = conv_store_->GetList("owner");
    auto convs_member = conv_store_->GetList("member");
    for (const auto& c : convs_owner) {
        EXPECT_NE(c.conversation_id, "g1");
    }
    for (const auto& c : convs_member) {
        EXPECT_NE(c.conversation_id, "g1");
    }

    // 群状态为已解散，历史应为空
    auto g_after = group_store_->GetGroup("g1");
    ASSERT_TRUE(g_after.has_value());
    EXPECT_EQ(g_after->status, 1);

    auto history = service_->GetHistory("owner", "g1", ChatType::GROUP, "", 50);
    EXPECT_TRUE(history.empty());
}

} // namespace swift::chat

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

