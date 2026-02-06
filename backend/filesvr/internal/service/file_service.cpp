/**
 * @file file_service.cpp
 * @brief FileSvr 业务逻辑：上传会话、断点续传、文件元信息与本地存储
 */

#include "file_service.h"
#include "swift/error_code.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

namespace fs = std::filesystem;

namespace swift::file {

FileServiceCore::FileServiceCore(std::shared_ptr<FileStore> store,
                         const FileConfig &config)
    : store_(std::move(store)), config_(config) {}

FileServiceCore::~FileServiceCore() = default;

namespace {

std::string HexEncode(const unsigned char *data, size_t len) {
  static const char kHex[] = "0123456789abcdef";
  std::string out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; ++i) {
    out.push_back(kHex[(data[i] >> 4) & 0x0f]);
    out.push_back(kHex[data[i] & 0x0f]);
  }
  return out;
}

} // namespace

std::string FileServiceCore::GenerateFileId() {
  auto now = std::chrono::system_clock::now().time_since_epoch().count();
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dis;
  uint64_t r = dis(gen);
  unsigned char buf[16];
  memcpy(buf, &now, 8);
  memcpy(buf + 8, &r, 8);
  return HexEncode(buf, 16);
}

std::string FileServiceCore::BuildStoragePath(const std::string &file_id) {
  if (file_id.size() < 2)
    return config_.storage_path + "/" + file_id;
  return config_.storage_path + "/" + file_id.substr(0, 2) + "/" + file_id;
}

std::string FileServiceCore::BuildFileUrl(const std::string &file_id) {
  std::string host = config_.host;
  if (host == "0.0.0.0")
    host = "127.0.0.1";
  return "http://" + host + ":" + std::to_string(config_.http_port) +
         "/files/" + file_id;
}

std::string FileServiceCore::GetTempPath(const std::string &upload_id) {
  return config_.storage_path + "/.tmp/" + upload_id;
}

int64_t FileServiceCore::NowSeconds() const {
  return static_cast<int64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

// -----------------------------------------------------------------------------
// InitUpload
// -----------------------------------------------------------------------------
FileServiceCore::InitUploadResult FileServiceCore::InitUpload(const std::string &user_id,
                        const std::string &file_name,
                        const std::string &content_type, int64_t file_size,
                        const std::string &md5, const std::string &msg_id) {
  InitUploadResult out;
  if (file_size <= 0 || file_size > config_.max_file_size) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::FILE_TOO_LARGE);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::FILE_TOO_LARGE);
    return out;
  }

  if (!md5.empty()) {
    auto existing = store_->GetByMd5(md5);
    if (existing) {
      out.success = true;
      out.existing_file_id = existing->file_id;
      out.upload_id = existing->file_id;
      out.expire_at = NowSeconds() + config_.upload_session_expire_seconds;
      return out;
    }
  }

  std::string upload_id = GenerateFileId();
  int64_t expire_at = NowSeconds() + config_.upload_session_expire_seconds;
  std::string temp_path = GetTempPath(upload_id);

  fs::path tmp_dir = fs::path(config_.storage_path) / ".tmp";
  std::error_code ec;
  fs::create_directories(tmp_dir, ec);
  if (ec) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::INTERNAL_ERROR);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::INTERNAL_ERROR);
    return out;
  }

  UploadSessionData session;
  session.upload_id = upload_id;
  session.user_id = user_id;
  session.file_name = file_name;
  session.content_type = content_type;
  session.file_size = file_size;
  session.md5 = md5;
  session.msg_id = msg_id;
  session.temp_path = temp_path;
  session.bytes_written = 0;
  session.expire_at = expire_at;

  if (!store_->SaveUploadSession(session)) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::UPLOAD_FAILED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::UPLOAD_FAILED);
    return out;
  }

  out.success = true;
  out.upload_id = upload_id;
  out.expire_at = expire_at;
  return out;
}

// -----------------------------------------------------------------------------
// GetUploadState
// -----------------------------------------------------------------------------
FileServiceCore::GetUploadStateResult FileServiceCore::GetUploadState(const std::string &upload_id) {
  GetUploadStateResult out;
  auto s = store_->GetUploadSession(upload_id);
  if (!s) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::FILE_EXPIRED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::FILE_EXPIRED);
    return out;
  }
  out.found = true;
  out.offset = s->bytes_written;
  out.file_size = s->file_size;
  out.expire_at = s->expire_at;
  out.completed = false;
  return out;
}

// -----------------------------------------------------------------------------
// AppendChunk
// -----------------------------------------------------------------------------
FileServiceCore::AppendChunkResult FileServiceCore::AppendChunk(const std::string &upload_id, const void *data,
                         size_t size) {
  AppendChunkResult out;
  auto s = store_->GetUploadSession(upload_id);
  if (!s) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::FILE_EXPIRED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::FILE_EXPIRED);
    return out;
  }
  if (s->bytes_written + static_cast<int64_t>(size) > s->file_size) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::INVALID_PARAM);
    return out;
  }

  fs::path path(s->temp_path);
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  std::ofstream f(path, std::ios::binary | std::ios::app);
  if (!f) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::UPLOAD_FAILED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::UPLOAD_FAILED);
    return out;
  }
  f.write(static_cast<const char *>(data), static_cast<std::streamsize>(size));
  if (!f) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::UPLOAD_FAILED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::UPLOAD_FAILED);
    return out;
  }
  f.flush();
  if (!f) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::UPLOAD_FAILED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::UPLOAD_FAILED);
    return out;
  }
  int64_t new_offset = s->bytes_written + static_cast<int64_t>(size);
  if (!store_->UpdateUploadSessionBytes(upload_id, new_offset)) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::UPLOAD_FAILED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::UPLOAD_FAILED);
    return out;
  }

  out.success = true;
  out.new_offset = new_offset;
  return out;
}

// -----------------------------------------------------------------------------
// CompleteUpload
// -----------------------------------------------------------------------------
FileServiceCore::CompleteUploadResult
FileServiceCore::CompleteUpload(const std::string &upload_id) {
  CompleteUploadResult out;
  auto s = store_->GetUploadSession(upload_id);
  if (!s) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::FILE_EXPIRED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::FILE_EXPIRED);
    return out;
  }
  if (s->bytes_written != s->file_size) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::UPLOAD_INCOMPLETE);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::UPLOAD_INCOMPLETE);
    return out;
  }

  std::string file_id = GenerateFileId();
  std::string final_path = BuildStoragePath(file_id);

  std::error_code ec;
  fs::path final_p(final_path);
  fs::create_directories(final_p.parent_path(), ec);
  if (ec) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::INTERNAL_ERROR);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::INTERNAL_ERROR);
    return out;
  }
  fs::rename(s->temp_path, final_path, ec);
  if (ec) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::UPLOAD_FAILED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::UPLOAD_FAILED);
    return out;
  }

  FileMetaData meta;
  meta.file_id = file_id;
  meta.file_name = s->file_name;
  meta.content_type = s->content_type;
  meta.file_size = s->file_size;
  meta.md5 = s->md5;
  meta.uploader_id = s->user_id;
  meta.storage_path = final_path;
  meta.uploaded_at = NowSeconds();

  if (!store_->Save(meta)) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::UPLOAD_FAILED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::UPLOAD_FAILED);
    fs::remove(final_path, ec);
    return out;
  }

  store_->DeleteUploadSession(upload_id);

  out.success = true;
  out.file_id = file_id;
  out.file_url = BuildFileUrl(file_id);
  return out;
}

// -----------------------------------------------------------------------------
// Upload (one-shot)
// -----------------------------------------------------------------------------
FileServiceCore::UploadResult FileServiceCore::Upload(const std::string &user_id,
                                              const std::string &file_name,
                                              const std::string &content_type,
                                              const std::vector<char> &data) {
  UploadResult out;
  if (static_cast<int64_t>(data.size()) > config_.max_file_size) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::FILE_TOO_LARGE);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::FILE_TOO_LARGE);
    return out;
  }

  std::string file_id = GenerateFileId();
  std::string path = BuildStoragePath(file_id);

  std::error_code ec;
  fs::path p(path);
  fs::create_directories(p.parent_path(), ec);
  if (ec) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::INTERNAL_ERROR);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::INTERNAL_ERROR);
    return out;
  }

  std::ofstream f(path, std::ios::binary);
  if (!f) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::UPLOAD_FAILED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::UPLOAD_FAILED);
    return out;
  }
  f.write(data.data(), static_cast<std::streamsize>(data.size()));
  if (!f) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::UPLOAD_FAILED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::UPLOAD_FAILED);
    fs::remove(path, ec);
    return out;
  }

  FileMetaData meta;
  meta.file_id = file_id;
  meta.file_name = file_name;
  meta.content_type = content_type;
  meta.file_size = static_cast<int64_t>(data.size());
  meta.uploader_id = user_id;
  meta.storage_path = path;
  meta.uploaded_at = NowSeconds();

  if (!store_->Save(meta)) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::UPLOAD_FAILED);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::UPLOAD_FAILED);
    fs::remove(path, ec);
    return out;
  }

  out.success = true;
  out.file_id = file_id;
  out.file_url = BuildFileUrl(file_id);
  return out;
}

// -----------------------------------------------------------------------------
// GetFileUrl
// -----------------------------------------------------------------------------
FileServiceCore::FileUrlResult FileServiceCore::GetFileUrl(const std::string &file_id,
                                                   const std::string &user_id) {
  FileUrlResult out;
  (void)user_id;
  auto meta = store_->GetById(file_id);
  if (!meta) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::FILE_NOT_FOUND);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::FILE_NOT_FOUND);
    return out;
  }
  out.success = true;
  out.file_url = BuildFileUrl(file_id);
  out.file_name = meta->file_name;
  out.file_size = meta->file_size;
  out.content_type = meta->content_type;
  out.expire_at = 0;
  return out;
}

// -----------------------------------------------------------------------------
// ReadFile
// -----------------------------------------------------------------------------
bool FileServiceCore::ReadFile(const std::string &file_id, std::vector<char> &data,
                           std::string &content_type, std::string &file_name) {
  auto meta = store_->GetById(file_id);
  if (!meta)
    return false;
  std::ifstream f(meta->storage_path, std::ios::binary | std::ios::ate);
  if (!f)
    return false;
  auto size = f.tellg();
  f.seekg(0);
  data.resize(static_cast<size_t>(size));
  if (!f.read(data.data(), size))
    return false;
  content_type = meta->content_type;
  file_name = meta->file_name;
  return true;
}

// -----------------------------------------------------------------------------
// ReadFileRange
// -----------------------------------------------------------------------------
bool FileServiceCore::ReadFileRange(const std::string &file_id, int64_t offset,
                                int64_t length, std::vector<char> &data,
                                std::string &content_type, std::string &file_name,
                                int64_t &file_size) {
  auto meta = store_->GetById(file_id);
  if (!meta)
    return false;
  file_size = meta->file_size;
  content_type = meta->content_type;
  file_name = meta->file_name;
  if (offset < 0)
    offset = 0;
  if (offset >= file_size) {
    data.clear();
    return true;
  }
  int64_t read_len = length < 0 ? (file_size - offset) : length;
  if (offset + read_len > file_size)
    read_len = file_size - offset;
  std::ifstream f(meta->storage_path, std::ios::binary);
  if (!f)
    return false;
  f.seekg(offset);
  data.resize(static_cast<size_t>(read_len));
  if (!f.read(data.data(), read_len))
    return false;
  return true;
}

// -----------------------------------------------------------------------------
// GetFileInfo
// -----------------------------------------------------------------------------
FileServiceCore::FileInfoResult
FileServiceCore::GetFileInfo(const std::string &file_id) {
  FileInfoResult out;
  auto meta = store_->GetById(file_id);
  if (!meta)
    return out;
  out.found = true;
  out.meta = *meta;
  return out;
}

// -----------------------------------------------------------------------------
// DeleteFile
// -----------------------------------------------------------------------------
bool FileServiceCore::DeleteFile(const std::string &file_id,
                             const std::string &user_id) {
  auto meta = store_->GetById(file_id);
  if (!meta)
    return false;
  if (meta->uploader_id != user_id)
    return false;
  if (!store_->Delete(file_id))
    return false;
  std::error_code ec;
  fs::remove(meta->storage_path, ec);
  return true;
}

// -----------------------------------------------------------------------------
// CheckMd5
// -----------------------------------------------------------------------------
std::optional<std::string> FileServiceCore::CheckMd5(const std::string &md5) {
  auto meta = store_->GetByMd5(md5);
  if (!meta)
    return std::nullopt;
  return meta->file_id;
}

// -----------------------------------------------------------------------------
// GetUploadToken
// -----------------------------------------------------------------------------
FileServiceCore::UploadTokenResult
FileServiceCore::GetUploadToken(const std::string &user_id,
                            const std::string &file_name, int64_t file_size) {
  UploadTokenResult out;
  if (file_size <= 0 || file_size > config_.max_file_size) {
    out.error_code = swift::ErrorCodeToInt(swift::ErrorCode::FILE_TOO_LARGE);
    out.error = swift::ErrorCodeToString(swift::ErrorCode::FILE_TOO_LARGE);
    return out;
  }
  auto init = InitUpload(user_id, file_name, "", file_size, "", "");
  if (!init.success) {
    out.error_code = init.error_code;
    out.error = init.error;
    return out;
  }
  out.success = true;
  out.upload_token = init.upload_id;
  out.upload_url = BuildFileUrl(""); // 直传时由客户端使用 gRPC UploadFile
  if (out.upload_url.back() != '/')
    out.upload_url += "/";
  out.upload_url += "upload";
  out.expire_at = init.expire_at;
  return out;
}

} // namespace swift::file
