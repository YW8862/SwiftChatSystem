/**
 * @file file_store_test.cpp
 * @brief FileStore (RocksDBFileStore) 单元测试
 */

#include "file_store.h"
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>

namespace swift::file {

class FileStoreTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_db_path_ = "/tmp/file_store_test_" +
        std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    store_ = std::make_unique<RocksDBFileStore>(test_db_path_);
  }

  void TearDown() override {
    store_.reset();
    std::error_code ec;
    std::filesystem::remove_all(test_db_path_, ec);
  }

  FileMetaData MakeMeta(const std::string& suffix = "") {
    FileMetaData m;
    m.file_id = "fid_" + suffix;
    m.file_name = "test" + suffix + ".txt";
    m.content_type = "text/plain";
    m.file_size = 100 + suffix.size();
    m.md5 = "md5_" + suffix;
    m.uploader_id = "user1";
    m.storage_path = "/tmp/storage/fid_" + suffix;
    m.uploaded_at = 1700000000;
    return m;
  }

  UploadSessionData MakeSession(const std::string& suffix = "") {
    UploadSessionData s;
    s.upload_id = "up_" + suffix;
    s.user_id = "user1";
    s.file_name = "file" + suffix + ".bin";
    s.content_type = "application/octet-stream";
    s.file_size = 1024;
    s.md5 = "";
    s.msg_id = "";
    s.temp_path = "/tmp/upload/up_" + suffix;
    s.bytes_written = 0;
    s.expire_at = 1700003600;
    return s;
  }

  std::string test_db_path_;
  std::unique_ptr<RocksDBFileStore> store_;
};

// -----------------------------------------------------------------------------
// FileMetaData: Save / GetById
// -----------------------------------------------------------------------------

TEST_F(FileStoreTest, Save_GetById_Success) {
  FileMetaData m = MakeMeta("1");
  EXPECT_TRUE(store_->Save(m));

  auto got = store_->GetById(m.file_id);
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(got->file_id, m.file_id);
  EXPECT_EQ(got->file_name, m.file_name);
  EXPECT_EQ(got->content_type, m.content_type);
  EXPECT_EQ(got->file_size, m.file_size);
  EXPECT_EQ(got->md5, m.md5);
  EXPECT_EQ(got->uploader_id, m.uploader_id);
  EXPECT_EQ(got->storage_path, m.storage_path);
  EXPECT_EQ(got->uploaded_at, m.uploaded_at);
}

TEST_F(FileStoreTest, Save_EmptyFileId_Fail) {
  FileMetaData m = MakeMeta();
  m.file_id = "";
  EXPECT_FALSE(store_->Save(m));
}

TEST_F(FileStoreTest, GetById_NotExists) {
  auto got = store_->GetById("nonexistent");
  EXPECT_FALSE(got.has_value());
}

TEST_F(FileStoreTest, GetById_EmptyId) {
  auto got = store_->GetById("");
  EXPECT_FALSE(got.has_value());
}

// -----------------------------------------------------------------------------
// GetByMd5
// -----------------------------------------------------------------------------

TEST_F(FileStoreTest, Save_GetByMd5_Success) {
  FileMetaData m = MakeMeta("md5a");
  m.md5 = "abc123";
  EXPECT_TRUE(store_->Save(m));

  auto got = store_->GetByMd5("abc123");
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(got->file_id, m.file_id);
  EXPECT_EQ(got->md5, "abc123");
}

TEST_F(FileStoreTest, GetByMd5_NotExists) {
  auto got = store_->GetByMd5("nomd5");
  EXPECT_FALSE(got.has_value());
}

TEST_F(FileStoreTest, GetByMd5_EmptyMd5) {
  auto got = store_->GetByMd5("");
  EXPECT_FALSE(got.has_value());
}

// -----------------------------------------------------------------------------
// Delete
// -----------------------------------------------------------------------------

TEST_F(FileStoreTest, Delete_Success) {
  FileMetaData m = MakeMeta("del");
  EXPECT_TRUE(store_->Save(m));
  EXPECT_TRUE(store_->GetById(m.file_id).has_value());

  EXPECT_TRUE(store_->Delete(m.file_id));
  EXPECT_FALSE(store_->GetById(m.file_id).has_value());
  EXPECT_FALSE(store_->GetByMd5(m.md5).has_value());
}

TEST_F(FileStoreTest, Delete_NotExists) {
  EXPECT_FALSE(store_->Delete("nonexistent"));
}

TEST_F(FileStoreTest, Delete_EmptyId) {
  EXPECT_FALSE(store_->Delete(""));
}

// -----------------------------------------------------------------------------
// UploadSession: SaveUploadSession / GetUploadSession
// -----------------------------------------------------------------------------

TEST_F(FileStoreTest, SaveUploadSession_GetUploadSession_Success) {
  UploadSessionData s = MakeSession("s1");
  EXPECT_TRUE(store_->SaveUploadSession(s));

  auto got = store_->GetUploadSession(s.upload_id);
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(got->upload_id, s.upload_id);
  EXPECT_EQ(got->user_id, s.user_id);
  EXPECT_EQ(got->file_name, s.file_name);
  EXPECT_EQ(got->file_size, s.file_size);
  EXPECT_EQ(got->bytes_written, 0);
  EXPECT_EQ(got->expire_at, s.expire_at);
}

TEST_F(FileStoreTest, SaveUploadSession_EmptyUploadId_Fail) {
  UploadSessionData s = MakeSession();
  s.upload_id = "";
  EXPECT_FALSE(store_->SaveUploadSession(s));
}

TEST_F(FileStoreTest, GetUploadSession_NotExists) {
  auto got = store_->GetUploadSession("no_such_upload");
  EXPECT_FALSE(got.has_value());
}

TEST_F(FileStoreTest, GetUploadSession_EmptyId) {
  auto got = store_->GetUploadSession("");
  EXPECT_FALSE(got.has_value());
}

// -----------------------------------------------------------------------------
// UpdateUploadSessionBytes
// -----------------------------------------------------------------------------

TEST_F(FileStoreTest, UpdateUploadSessionBytes_Success) {
  UploadSessionData s = MakeSession("bytes");
  EXPECT_TRUE(store_->SaveUploadSession(s));

  EXPECT_TRUE(store_->UpdateUploadSessionBytes(s.upload_id, 512));

  auto got = store_->GetUploadSession(s.upload_id);
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(got->bytes_written, 512);
}

TEST_F(FileStoreTest, UpdateUploadSessionBytes_NotExists) {
  EXPECT_FALSE(store_->UpdateUploadSessionBytes("no_such", 100));
}

// -----------------------------------------------------------------------------
// DeleteUploadSession
// -----------------------------------------------------------------------------

TEST_F(FileStoreTest, DeleteUploadSession_Success) {
  UploadSessionData s = MakeSession("delup");
  EXPECT_TRUE(store_->SaveUploadSession(s));
  EXPECT_TRUE(store_->GetUploadSession(s.upload_id).has_value());

  EXPECT_TRUE(store_->DeleteUploadSession(s.upload_id));
  EXPECT_FALSE(store_->GetUploadSession(s.upload_id).has_value());
}

TEST_F(FileStoreTest, DeleteUploadSession_NotExists) {
  EXPECT_TRUE(store_->DeleteUploadSession("no_such"));  // 实现可能返回 true
}

// -----------------------------------------------------------------------------
// 综合
// -----------------------------------------------------------------------------

TEST_F(FileStoreTest, MultipleMetaAndSessions) {
  for (int i = 0; i < 5; ++i) {
    FileMetaData m = MakeMeta(std::to_string(i));
    EXPECT_TRUE(store_->Save(m));
    UploadSessionData s = MakeSession(std::to_string(i));
    EXPECT_TRUE(store_->SaveUploadSession(s));
  }
  for (int i = 0; i < 5; ++i) {
    auto m = store_->GetById("fid_" + std::to_string(i));
    ASSERT_TRUE(m.has_value());
    auto s = store_->GetUploadSession("up_" + std::to_string(i));
    ASSERT_TRUE(s.has_value());
  }
}

TEST_F(FileStoreTest, PersistenceAcrossReopen) {
  FileMetaData m = MakeMeta("persist");
  EXPECT_TRUE(store_->Save(m));
  store_.reset();

  store_ = std::make_unique<RocksDBFileStore>(test_db_path_);
  auto got = store_->GetById(m.file_id);
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(got->file_name, m.file_name);
}

}  // namespace swift::file

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
