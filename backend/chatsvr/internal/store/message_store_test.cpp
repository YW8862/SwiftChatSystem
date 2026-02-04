/**
 * @file message_store_test.cpp
 * @brief RocksDBMessageStore 单元测试（消息、历史、撤回、离线队列）
 */

#include "message_store.h"
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>

namespace swift::chat {

class MessageStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto suffix = std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
        db_path_ = "/tmp/messagestore_test_" + suffix;
        store_ = std::make_unique<RocksDBMessageStore>(db_path_);
    }

    void TearDown() override {
        store_.reset();
        std::filesystem::remove_all(db_path_);
    }

    MessageData MakeMessage(const std::string& conv_id,
                            const std::string& msg_id,
                            int64_t ts) {
        MessageData m;
        m.msg_id = msg_id;
        m.from_user_id = "u1";
        m.to_id = "u2";
        m.conversation_id = conv_id;
        m.chat_type = 1;
        m.content = "hello " + msg_id;
        m.media_url = "";
        m.media_type = "text";
        m.timestamp = ts;
        m.status = 0;
        m.recall_at = 0;
        return m;
    }

    std::string db_path_;
    std::unique_ptr<RocksDBMessageStore> store_;
};

// 保存与按 ID 查询
TEST_F(MessageStoreTest, SaveAndGetById) {
    MessageData m = MakeMessage("c1", "m1", 1000);
    ASSERT_TRUE(store_->Save(m));

    auto got = store_->GetById("m1");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->msg_id, "m1");
    EXPECT_EQ(got->conversation_id, "c1");
    EXPECT_EQ(got->content, "hello m1");
    EXPECT_EQ(got->status, 0);
}

TEST_F(MessageStoreTest, GetById_NotExists) {
    auto got = store_->GetById("not_exists");
    EXPECT_FALSE(got.has_value());
}

// 会话历史：倒序、分页
TEST_F(MessageStoreTest, GetHistory_OrderAndLimit) {
    std::string conv = "c_history";
    // 时间依次递增，最新的在最后保存
    ASSERT_TRUE(store_->Save(MakeMessage(conv, "m1", 1000)));
    ASSERT_TRUE(store_->Save(MakeMessage(conv, "m2", 2000)));
    ASSERT_TRUE(store_->Save(MakeMessage(conv, "m3", 3000)));

    // 不指定 before_msg_id，limit=2，应返回 [m3, m2]
    auto list = store_->GetHistory(conv, 1, "", 2);
    ASSERT_EQ(list.size(), 2u);
    EXPECT_EQ(list[0].msg_id, "m3");
    EXPECT_EQ(list[1].msg_id, "m2");

    // 指定 before_msg_id=m2，应返回 [m1]
    auto list2 = store_->GetHistory(conv, 1, "m2", 10);
    ASSERT_EQ(list2.size(), 1u);
    EXPECT_EQ(list2[0].msg_id, "m1");
}

// 撤回：status / recall_at 更新
TEST_F(MessageStoreTest, MarkRecalled_UpdatesStatusAndRecallAt) {
    MessageData m = MakeMessage("c_recall", "m_recall", 1000);
    ASSERT_TRUE(store_->Save(m));

    int64_t recall_ts = 5000;
    ASSERT_TRUE(store_->MarkRecalled("m_recall", recall_ts));

    auto got = store_->GetById("m_recall");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->status, 1);
    EXPECT_EQ(got->recall_at, recall_ts);
}

// 离线队列：AddToOffline / PullOffline / ClearOffline
TEST_F(MessageStoreTest, OfflineQueue_PullAndClear) {
    std::string conv = "c_offline";
    // 先保存消息
    ASSERT_TRUE(store_->Save(MakeMessage(conv, "m1", 1000)));
    ASSERT_TRUE(store_->Save(MakeMessage(conv, "m2", 2000)));
    ASSERT_TRUE(store_->Save(MakeMessage(conv, "m3", 3000)));

    std::string user = "u_offline";
    EXPECT_TRUE(store_->AddToOffline(user, "m1"));
    EXPECT_TRUE(store_->AddToOffline(user, "m2"));
    EXPECT_TRUE(store_->AddToOffline(user, "m3"));

    std::string next_cursor;
    bool has_more = false;

    // 拉取所有离线，应按时间倒序 m3, m2, m1
    auto all = store_->PullOffline(user, "", 10, next_cursor, has_more);
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all[0].msg_id, "m3");
    EXPECT_EQ(all[1].msg_id, "m2");
    EXPECT_EQ(all[2].msg_id, "m1");

    // ClearOffline 到 m3，应清除所有离线记录
    EXPECT_TRUE(store_->ClearOffline(user, "m3"));
    auto empty = store_->PullOffline(user, "", 10, next_cursor, has_more);
    EXPECT_TRUE(empty.empty());
}

} // namespace swift::chat

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

