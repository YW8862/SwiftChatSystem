/**
 * @file file_store.cpp
 * @brief RocksDB 文件元信息存储实现
 *
 * Key 设计：
 *   file:{file_id}     -> FileMetaData JSON
 *   file_md5:{md5}     -> file_id（用于秒传检测）
 */

#include "file_store.h"
#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>
#include <stdexcept>

using json = nlohmann::json;

namespace swift::file {

namespace {

std::string SerializeMeta(const FileMetaData& meta) {
  json j;
  j["file_id"] = meta.file_id;
  j["file_name"] = meta.file_name;
  j["content_type"] = meta.content_type;
  j["file_size"] = meta.file_size;
  j["md5"] = meta.md5;
  j["uploader_id"] = meta.uploader_id;
  j["storage_path"] = meta.storage_path;
  j["uploaded_at"] = meta.uploaded_at;
  return j.dump();
}

FileMetaData DeserializeMeta(const std::string& data) {
  json j = json::parse(data);
  FileMetaData meta;
  meta.file_id = j.value("file_id", "");
  meta.file_name = j.value("file_name", "");
  meta.content_type = j.value("content_type", "");
  meta.file_size = j.value("file_size", static_cast<int64_t>(0));
  meta.md5 = j.value("md5", "");
  meta.uploader_id = j.value("uploader_id", "");
  meta.storage_path = j.value("storage_path", "");
  meta.uploaded_at = j.value("uploaded_at", static_cast<int64_t>(0));
  return meta;
}

std::string SerializeSession(const UploadSessionData& s) {
  json j;
  j["upload_id"] = s.upload_id;
  j["user_id"] = s.user_id;
  j["file_name"] = s.file_name;
  j["content_type"] = s.content_type;
  j["file_size"] = s.file_size;
  j["md5"] = s.md5;
  j["msg_id"] = s.msg_id;
  j["temp_path"] = s.temp_path;
  j["bytes_written"] = s.bytes_written;
  j["expire_at"] = s.expire_at;
  return j.dump();
}

UploadSessionData DeserializeSession(const std::string& data) {
  json j = json::parse(data);
  UploadSessionData s;
  s.upload_id = j.value("upload_id", "");
  s.user_id = j.value("user_id", "");
  s.file_name = j.value("file_name", "");
  s.content_type = j.value("content_type", "");
  s.file_size = j.value("file_size", static_cast<int64_t>(0));
  s.md5 = j.value("md5", "");
  s.msg_id = j.value("msg_id", "");
  s.temp_path = j.value("temp_path", "");
  s.bytes_written = j.value("bytes_written", static_cast<int64_t>(0));
  s.expire_at = j.value("expire_at", static_cast<int64_t>(0));
  return s;
}

constexpr const char* KEY_PREFIX_FILE = "file:";
constexpr const char* KEY_PREFIX_FILE_MD5 = "file_md5:";
constexpr const char* KEY_PREFIX_UPLOAD = "upload:";

}  // namespace

struct RocksDBFileStore::Impl {
  rocksdb::DB* db = nullptr;
  std::string db_path;

  ~Impl() {
    if (db) {
      delete db;
      db = nullptr;
    }
  }
};

RocksDBFileStore::RocksDBFileStore(const std::string& db_path)
    : impl_(std::make_unique<Impl>()) {
  impl_->db_path = db_path;

  rocksdb::Options options;
  options.create_if_missing = true;
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();

  rocksdb::Status status = rocksdb::DB::Open(options, db_path, &impl_->db);
  if (!status.ok()) {
    throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
  }
}

RocksDBFileStore::~RocksDBFileStore() = default;

// 保存文件元信息
bool RocksDBFileStore::Save(const FileMetaData& meta) {
  if (!impl_->db || meta.file_id.empty())
    return false;

  std::string value = SerializeMeta(meta);
  rocksdb::WriteBatch batch;
  batch.Put(KEY_PREFIX_FILE + meta.file_id, value);

  if (!meta.md5.empty()) {
    batch.Put(KEY_PREFIX_FILE_MD5 + meta.md5, meta.file_id);
  }

  rocksdb::WriteOptions write_opts;
  write_opts.sync = true;
  rocksdb::Status status = impl_->db->Write(write_opts, &batch);
  return status.ok();
}

// 根据文件 ID 获取文件元信息
std::optional<FileMetaData> RocksDBFileStore::GetById(const std::string& file_id) {
  if (!impl_->db || file_id.empty())
    return std::nullopt;

  std::string value;
  rocksdb::Status status =
      impl_->db->Get(rocksdb::ReadOptions(), KEY_PREFIX_FILE + file_id, &value);

  if (!status.ok())
    return std::nullopt;

  try {
    return DeserializeMeta(value);
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

// 根据 MD5 获取文件元信息
std::optional<FileMetaData> RocksDBFileStore::GetByMd5(const std::string& md5) {
  if (!impl_->db || md5.empty())
    return std::nullopt;

  std::string file_id;
  rocksdb::Status status =
      impl_->db->Get(rocksdb::ReadOptions(), KEY_PREFIX_FILE_MD5 + md5, &file_id);

  if (!status.ok())
    return std::nullopt;

  return GetById(file_id);
}

// 删除文件元信息
bool RocksDBFileStore::Delete(const std::string& file_id) {
  if (!impl_->db || file_id.empty())
    return false;

  auto meta = GetById(file_id);
  if (!meta)
    return false;

  rocksdb::WriteBatch batch;
  batch.Delete(KEY_PREFIX_FILE + file_id);
  if (!meta->md5.empty()) {
    batch.Delete(KEY_PREFIX_FILE_MD5 + meta->md5);
  }

  rocksdb::WriteOptions write_opts;
  write_opts.sync = true;
  rocksdb::Status status = impl_->db->Write(write_opts, &batch);
  return status.ok();
}

bool RocksDBFileStore::SaveUploadSession(const UploadSessionData& session) {
  if (!impl_->db || session.upload_id.empty())
    return false;
  std::string value = SerializeSession(session);
  rocksdb::WriteOptions write_opts;
  write_opts.sync = true;
  return impl_->db->Put(write_opts, KEY_PREFIX_UPLOAD + session.upload_id, value).ok();
}

std::optional<UploadSessionData> RocksDBFileStore::GetUploadSession(const std::string& upload_id) {
  if (!impl_->db || upload_id.empty())
    return std::nullopt;
  std::string value;
  if (!impl_->db->Get(rocksdb::ReadOptions(), KEY_PREFIX_UPLOAD + upload_id, &value).ok())
    return std::nullopt;
  try {
    return DeserializeSession(value);
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

bool RocksDBFileStore::UpdateUploadSessionBytes(const std::string& upload_id, int64_t bytes_written) {
  if (!impl_->db || upload_id.empty())
    return false;
  auto s = GetUploadSession(upload_id);
  if (!s)
    return false;
  s->bytes_written = bytes_written;
  return SaveUploadSession(*s);
}

bool RocksDBFileStore::DeleteUploadSession(const std::string& upload_id) {
  if (!impl_->db || upload_id.empty())
    return false;
  rocksdb::WriteOptions write_opts;
  write_opts.sync = true;
  return impl_->db->Delete(write_opts, KEY_PREFIX_UPLOAD + upload_id).ok();
}

}  // namespace swift::file
