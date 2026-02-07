/**
 * @file file_service_test.cpp
 * @brief FileServiceCore 单元测试
 */

#include "../store/file_store.h"
#include "file_service.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace swift::file {

class FileServiceTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto suffix = std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count());
    base_path_ = "/tmp/file_service_test_" + suffix;
    db_path_ = base_path_ + "/meta";
    storage_path_ = base_path_ + "/storage";
    std::filesystem::create_directories(db_path_);
    std::filesystem::create_directories(storage_path_);
    std::filesystem::create_directories(storage_path_ + "/.tmp");

    store_ = std::make_shared<RocksDBFileStore>(db_path_);
    config_.host = "127.0.0.1";
    config_.grpc_port = 9100;
    config_.http_port = 8080;
    config_.storage_path = storage_path_;
    config_.max_file_size = 10 * 1024 * 1024;  // 10MB
    config_.upload_session_expire_seconds = 3600;
    service_ = std::make_unique<FileServiceCore>(store_, config_);
  }

  void TearDown() override {
    service_.reset();
    store_.reset();
    std::error_code ec;
    std::filesystem::remove_all(base_path_, ec);
  }

  std::string base_path_;
  std::string db_path_;
  std::string storage_path_;
  FileConfig config_;
  std::shared_ptr<FileStore> store_;
  std::unique_ptr<FileServiceCore> service_;
};

// -----------------------------------------------------------------------------
// InitUpload
// -----------------------------------------------------------------------------

TEST_F(FileServiceTest, InitUpload_Success) {
  auto r = service_->InitUpload("user1", "test.txt", "text/plain", 100, "", "");
  EXPECT_TRUE(r.success) << r.error;
  EXPECT_FALSE(r.upload_id.empty());
  EXPECT_GT(r.expire_at, 0);
}

TEST_F(FileServiceTest, InitUpload_FileTooLarge_Fail) {
  auto r = service_->InitUpload("user1", "big.bin", "application/octet-stream",
                                config_.max_file_size + 1, "", "");
  EXPECT_FALSE(r.success);
  EXPECT_TRUE(r.upload_id.empty());
}

TEST_F(FileServiceTest, InitUpload_ZeroSize_Fail) {
  auto r = service_->InitUpload("user1", "empty.txt", "text/plain", 0, "", "");
  EXPECT_FALSE(r.success);
}

TEST_F(FileServiceTest, InitUpload_Md5Existing_InstantUpload) {
  FileMetaData existing;
  existing.file_id = "f_existing";
  existing.file_name = "existing.txt";
  existing.content_type = "text/plain";
  existing.file_size = 100;
  existing.md5 = "md5_abc123";
  existing.uploader_id = "user1";
  existing.storage_path = storage_path_ + "/f_existing";
  existing.uploaded_at = 1700000000;
  std::filesystem::create_directories(storage_path_);
  std::ofstream of(existing.storage_path);
  of.write("x", 1);
  of.close();
  ASSERT_TRUE(store_->Save(existing));

  auto r = service_->InitUpload("user2", "second.txt", "text/plain", 100,
                                "md5_abc123", "");
  EXPECT_TRUE(r.success);
  EXPECT_EQ(r.existing_file_id, "f_existing");
  EXPECT_EQ(r.upload_id, "f_existing");
}

// -----------------------------------------------------------------------------
// GetUploadState
// -----------------------------------------------------------------------------

TEST_F(FileServiceTest, GetUploadState_NotFound) {
  auto r = service_->GetUploadState("nonexistent_upload_id");
  EXPECT_FALSE(r.found);
}

TEST_F(FileServiceTest, GetUploadState_AfterInit) {
  auto init = service_->InitUpload("user1", "a.bin", "application/octet-stream",
                                   50, "", "");
  ASSERT_TRUE(init.success);

  auto r = service_->GetUploadState(init.upload_id);
  EXPECT_TRUE(r.found);
  EXPECT_EQ(r.offset, 0);
  EXPECT_EQ(r.file_size, 50);
  EXPECT_FALSE(r.completed);
  EXPECT_EQ(r.expire_at, init.expire_at);
}

// -----------------------------------------------------------------------------
// AppendChunk / CompleteUpload 完整流程
// -----------------------------------------------------------------------------

TEST_F(FileServiceTest, AppendChunk_CompleteUpload_FullFlow) {
  auto init = service_->InitUpload("user1", "chunked.bin", "application/octet-stream",
                                   10, "", "");
  ASSERT_TRUE(init.success);

  std::string chunk1 = "12345";
  auto a1 = service_->AppendChunk(init.upload_id, chunk1.data(), chunk1.size());
  EXPECT_TRUE(a1.success);
  EXPECT_EQ(a1.new_offset, 5);

  std::string chunk2 = "67890";
  auto a2 = service_->AppendChunk(init.upload_id, chunk2.data(), chunk2.size());
  EXPECT_TRUE(a2.success);
  EXPECT_EQ(a2.new_offset, 10);

  auto complete = service_->CompleteUpload(init.upload_id);
  EXPECT_TRUE(complete.success) << complete.error;
  EXPECT_FALSE(complete.file_id.empty());
  EXPECT_FALSE(complete.file_url.empty());

  auto info = service_->GetFileInfo(complete.file_id);
  EXPECT_TRUE(info.found);
  EXPECT_EQ(info.meta.file_size, 10);
  EXPECT_EQ(info.meta.file_name, "chunked.bin");
  EXPECT_EQ(info.meta.uploader_id, "user1");
}

TEST_F(FileServiceTest, CompleteUpload_Incomplete_Fail) {
  auto init = service_->InitUpload("user1", "partial.bin", "application/octet-stream",
                                   10, "", "");
  ASSERT_TRUE(init.success);
  service_->AppendChunk(init.upload_id, "123", 3);

  auto complete = service_->CompleteUpload(init.upload_id);
  EXPECT_FALSE(complete.success);
  EXPECT_TRUE(complete.file_id.empty());
}

TEST_F(FileServiceTest, AppendChunk_ExceedSize_Fail) {
  auto init = service_->InitUpload("user1", "over.bin", "application/octet-stream",
                                   5, "", "");
  ASSERT_TRUE(init.success);
  std::string big = "123456";
  auto a = service_->AppendChunk(init.upload_id, big.data(), big.size());
  EXPECT_FALSE(a.success);
}

// -----------------------------------------------------------------------------
// Upload (one-shot)
// -----------------------------------------------------------------------------

TEST_F(FileServiceTest, Upload_Success) {
  std::vector<char> data = {'h', 'i', '\n'};
  auto r = service_->Upload("user1", "hello.txt", "text/plain", data);
  EXPECT_TRUE(r.success) << r.error;
  EXPECT_FALSE(r.file_id.empty());
  EXPECT_FALSE(r.file_url.empty());

  auto info = service_->GetFileInfo(r.file_id);
  EXPECT_TRUE(info.found);
  EXPECT_EQ(info.meta.file_size, 3);
  EXPECT_EQ(info.meta.file_name, "hello.txt");
}

TEST_F(FileServiceTest, Upload_ThenReadFile) {
  std::vector<char> data = {'x', 'y', 'z'};
  auto up = service_->Upload("user1", "xyz.bin", "application/octet-stream", data);
  ASSERT_TRUE(up.success);

  std::vector<char> read_back;
  std::string ct, fn;
  EXPECT_TRUE(service_->ReadFile(up.file_id, read_back, ct, fn));
  EXPECT_EQ(read_back.size(), 3u);
  EXPECT_EQ(read_back[0], 'x');
  EXPECT_EQ(read_back[1], 'y');
  EXPECT_EQ(read_back[2], 'z');
  EXPECT_EQ(ct, "application/octet-stream");
  EXPECT_EQ(fn, "xyz.bin");
}

// -----------------------------------------------------------------------------
// GetFileUrl / GetFileInfo / DeleteFile
// -----------------------------------------------------------------------------

TEST_F(FileServiceTest, GetFileUrl_Success) {
  std::vector<char> data = {'a'};
  auto up = service_->Upload("user1", "a.txt", "text/plain", data);
  ASSERT_TRUE(up.success);

  auto url = service_->GetFileUrl(up.file_id, "user1");
  EXPECT_TRUE(url.success);
  EXPECT_FALSE(url.file_url.empty());
  EXPECT_EQ(url.file_name, "a.txt");
  EXPECT_EQ(url.file_size, 1);
}

TEST_F(FileServiceTest, GetFileUrl_NotFound) {
  auto url = service_->GetFileUrl("nonexistent_file_id", "user1");
  EXPECT_FALSE(url.success);
}

TEST_F(FileServiceTest, GetFileInfo_NotFound) {
  auto info = service_->GetFileInfo("nonexistent");
  EXPECT_FALSE(info.found);
}

TEST_F(FileServiceTest, DeleteFile_Success) {
  std::vector<char> data = {'d'};
  auto up = service_->Upload("user1", "del.txt", "text/plain", data);
  ASSERT_TRUE(up.success);

  EXPECT_TRUE(service_->DeleteFile(up.file_id, "user1"));
  auto info = service_->GetFileInfo(up.file_id);
  EXPECT_FALSE(info.found);
}

TEST_F(FileServiceTest, DeleteFile_WrongUser_Fail) {
  std::vector<char> data = {'d'};
  auto up = service_->Upload("user1", "del.txt", "text/plain", data);
  ASSERT_TRUE(up.success);

  EXPECT_FALSE(service_->DeleteFile(up.file_id, "user2"));
  auto info = service_->GetFileInfo(up.file_id);
  EXPECT_TRUE(info.found);
}

// -----------------------------------------------------------------------------
// CheckMd5 / GetUploadToken
// -----------------------------------------------------------------------------

TEST_F(FileServiceTest, CheckMd5_NoMatch) {
  auto fid = service_->CheckMd5("nomatch_md5");
  EXPECT_FALSE(fid.has_value());
}

TEST_F(FileServiceTest, GetUploadToken_Success) {
  auto r = service_->GetUploadToken("user1", "token.bin", 100);
  EXPECT_TRUE(r.success) << r.error;
  EXPECT_FALSE(r.upload_token.empty());
  EXPECT_GT(r.expire_at, 0);
}

TEST_F(FileServiceTest, GetUploadToken_FileTooLarge_Fail) {
  auto r = service_->GetUploadToken("user1", "big.bin", config_.max_file_size + 1);
  EXPECT_FALSE(r.success);
}

// -----------------------------------------------------------------------------
// ReadFileRange
// -----------------------------------------------------------------------------

TEST_F(FileServiceTest, ReadFileRange_Success) {
  std::vector<char> data = {'a', 'b', 'c', 'd', 'e'};
  auto up = service_->Upload("user1", "range.txt", "text/plain", data);
  ASSERT_TRUE(up.success);

  std::vector<char> out;
  std::string ct, fn;
  int64_t total = 0;
  bool ok = service_->ReadFileRange(up.file_id, 1, 3, out, ct, fn, total);
  EXPECT_TRUE(ok);
  EXPECT_EQ(total, 5);
  EXPECT_EQ(out.size(), 3u);
  EXPECT_EQ(out[0], 'b');
  EXPECT_EQ(out[1], 'c');
  EXPECT_EQ(out[2], 'd');
}

TEST_F(FileServiceTest, ReadFileRange_ToEnd) {
  std::vector<char> data = {'1', '2', '3'};
  auto up = service_->Upload("user1", "end.txt", "text/plain", data);
  ASSERT_TRUE(up.success);

  std::vector<char> out;
  std::string ct, fn;
  int64_t total = 0;
  bool ok = service_->ReadFileRange(up.file_id, 1, -1, out, ct, fn, total);
  EXPECT_TRUE(ok);
  EXPECT_EQ(total, 3);
  EXPECT_EQ(out.size(), 2u);
  EXPECT_EQ(out[0], '2');
  EXPECT_EQ(out[1], '3');
}

}  // namespace swift::file

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
