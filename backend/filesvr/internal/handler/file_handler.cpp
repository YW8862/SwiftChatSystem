/**
 * @file file_handler.cpp
 * @brief FileSvr gRPC/HTTP Handler：将请求转调 FileService，结果写入 proto 或 HTTP 响应。
 */

#include "../service/file_service.h"
#include "file_handler.h"
#include "swift/error_code.h"
#include "swift/grpc_auth.h"
#include <grpcpp/server_context.h>
#include <algorithm>
#include <cstring>

namespace swift::file {

namespace {

void SetOk(::swift::file::InitUploadResponse* r) {
    r->set_code(static_cast<int>(swift::ErrorCode::OK));
    r->set_message(swift::ErrorCodeToString(swift::ErrorCode::OK));
}

void SetFail(::swift::file::InitUploadResponse* r, int code, const std::string& msg) {
    r->set_code(code);
    r->set_message(msg);
}

template <typename R>
void SetResponseOk(R* response) {
    response->set_code(static_cast<int>(swift::ErrorCode::OK));
    response->set_message(swift::ErrorCodeToString(swift::ErrorCode::OK));
}

template <typename R>
void SetResponseFail(R* response, int code, const std::string& message) {
    response->set_code(code);
    response->set_message(message);
}

}  // namespace

FileHandler::FileHandler(std::shared_ptr<FileServiceCore> service, const std::string& jwt_secret)
    : service_(std::move(service)), jwt_secret_(jwt_secret) {}

FileHandler::~FileHandler() = default;

std::string FileHandler::GetUserId(::grpc::ServerContext* context,
                                   const std::string& request_user_id) const {
    if (!jwt_secret_.empty()) {
        std::string uid = swift::GetAuthenticatedUserId(context, jwt_secret_);
        if (!uid.empty())
            return uid;
    }
    return request_user_id;
}

::grpc::Status FileHandler::InitUpload(::grpc::ServerContext* context,
                                       const ::swift::file::InitUploadRequest* request,
                                       ::swift::file::InitUploadResponse* response) {
    std::string user_id = GetUserId(context, request->user_id());
    if (user_id.empty()) {
        SetFail(response, swift::ErrorCodeToInt(swift::ErrorCode::TOKEN_INVALID),
                "token invalid or missing");
        return ::grpc::Status::OK;
    }
    if (request->file_name().empty()) {
        SetFail(response, swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM),
                swift::ErrorCodeToString(swift::ErrorCode::INVALID_PARAM));
        return ::grpc::Status::OK;
    }
    auto result = service_->InitUpload(
        user_id, request->file_name(), request->content_type(), request->file_size(),
        request->md5(), request->msg_id());
    if (result.success) {
        SetOk(response);
        response->set_upload_id(result.upload_id.empty() ? result.existing_file_id : result.upload_id);
        response->set_expire_at(result.expire_at);
    } else {
        SetFail(response, result.error_code, result.error);
    }
    return ::grpc::Status::OK;
}

::grpc::Status FileHandler::GetUploadState(::grpc::ServerContext* context,
                                          const ::swift::file::GetUploadStateRequest* request,
                                          ::swift::file::GetUploadStateResponse* response) {
    (void)context;
    if (request->upload_id().empty()) {
        SetResponseFail(response, swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM),
                       swift::ErrorCodeToString(swift::ErrorCode::INVALID_PARAM));
        return ::grpc::Status::OK;
    }
    auto result = service_->GetUploadState(request->upload_id());
    if (!result.found) {
        response->set_code(result.error_code ? result.error_code
                                              : swift::ErrorCodeToInt(swift::ErrorCode::NOT_FOUND));
        response->set_message(result.error.empty() ? swift::ErrorCodeToString(swift::ErrorCode::NOT_FOUND)
                                                   : result.error);
        return ::grpc::Status::OK;
    }
    SetResponseOk(response);
    response->set_offset(result.offset);
    response->set_file_size(result.file_size);
    response->set_completed(result.completed);
    if (result.completed && !result.file_id.empty())
        response->set_file_id(result.file_id);
    response->set_expire_at(result.expire_at);
    return ::grpc::Status::OK;
}

::grpc::Status FileHandler::UploadFile(::grpc::ServerContext* context,
                                       ::grpc::ServerReader<::swift::file::UploadChunk>* reader,
                                       ::swift::file::UploadResponse* response) {
    (void)context;
    ::swift::file::UploadChunk chunk;
    std::string upload_id;
    int64_t write_offset = 0;
    bool first = true;

    while (reader->Read(&chunk)) {
        if (chunk.data_case() == ::swift::file::UploadChunk::kMeta) {
            const auto& m = chunk.meta();
            upload_id = m.upload_id();
            write_offset = 0;
            if (upload_id.empty()) {
                SetResponseFail(response, swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM),
                               "upload_id required");
                return ::grpc::Status::OK;
            }
            first = false;
            continue;
        }
        if (chunk.data_case() == ::swift::file::UploadChunk::kResumeMeta) {
            const auto& r = chunk.resume_meta();
            upload_id = r.upload_id();
            write_offset = r.offset();
            if (upload_id.empty()) {
                SetResponseFail(response, swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM),
                               "upload_id required");
                return ::grpc::Status::OK;
            }
            first = false;
            continue;
        }
        if (chunk.data_case() == ::swift::file::UploadChunk::kChunk) {
            const std::string& data = chunk.chunk();
            if (data.empty()) continue;
            if (upload_id.empty()) {
                SetResponseFail(response, swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM),
                               "first message must be meta or resume_meta");
                return ::grpc::Status::OK;
            }
            auto append = service_->AppendChunk(upload_id, data.data(), data.size());
            if (!append.success) {
                SetResponseFail(response, append.error_code, append.error);
                return ::grpc::Status::OK;
            }
            write_offset = append.new_offset;
        }
    }

    if (first || upload_id.empty()) {
        SetResponseFail(response, swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM),
                       "stream must start with meta or resume_meta");
        return ::grpc::Status::OK;
    }

    auto complete = service_->CompleteUpload(upload_id);
    if (complete.success) {
        SetResponseOk(response);
        response->set_file_id(complete.file_id);
        response->set_file_url(complete.file_url);
        if (!complete.thumbnail_url.empty())
            response->set_thumbnail_url(complete.thumbnail_url);
    } else {
        SetResponseFail(response, complete.error_code, complete.error);
    }
    return ::grpc::Status::OK;
}

::grpc::Status FileHandler::GetFileUrl(::grpc::ServerContext* context,
                                       const ::swift::file::GetFileUrlRequest* request,
                                       ::swift::file::FileUrlResponse* response) {
    std::string user_id = GetUserId(context, request->user_id());
    if (user_id.empty()) {
        SetResponseFail(response, swift::ErrorCodeToInt(swift::ErrorCode::TOKEN_INVALID),
                       "token invalid or missing");
        return ::grpc::Status::OK;
    }
    if (request->file_id().empty()) {
        SetResponseFail(response, swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM),
                       swift::ErrorCodeToString(swift::ErrorCode::INVALID_PARAM));
        return ::grpc::Status::OK;
    }
    auto result = service_->GetFileUrl(request->file_id(), user_id);
    if (result.success) {
        SetResponseOk(response);
        response->set_file_url(result.file_url);
        response->set_file_name(result.file_name);
        response->set_file_size(result.file_size);
        response->set_content_type(result.content_type);
        if (result.expire_at > 0)
            response->set_expire_at(result.expire_at);
    } else {
        SetResponseFail(response, result.error_code, result.error);
    }
    return ::grpc::Status::OK;
}

::grpc::Status FileHandler::GetFileInfo(::grpc::ServerContext* context,
                                        const ::swift::file::GetFileInfoRequest* request,
                                        ::swift::file::FileInfoResponse* response) {
    (void)context;
    if (request->file_id().empty()) {
        SetResponseFail(response, swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM),
                       swift::ErrorCodeToString(swift::ErrorCode::INVALID_PARAM));
        return ::grpc::Status::OK;
    }
    auto result = service_->GetFileInfo(request->file_id());
    if (!result.found) {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::FILE_NOT_FOUND));
        response->set_message(swift::ErrorCodeToString(swift::ErrorCode::FILE_NOT_FOUND));
        return ::grpc::Status::OK;
    }
    SetResponseOk(response);
    auto* info = response->mutable_file_info();
    info->set_file_id(result.meta.file_id);
    info->set_file_name(result.meta.file_name);
    info->set_file_size(result.meta.file_size);
    info->set_content_type(result.meta.content_type);
    info->set_uploader_id(result.meta.uploader_id);
    info->set_uploaded_at(result.meta.uploaded_at);
    info->set_md5(result.meta.md5);
    return ::grpc::Status::OK;
}

::grpc::Status FileHandler::DeleteFile(::grpc::ServerContext* context,
                                        const ::swift::file::DeleteFileRequest* request,
                                        ::swift::file::DeleteFileResponse* response) {
    std::string user_id = GetUserId(context, request->user_id());
    if (user_id.empty()) {
        SetResponseFail(response, swift::ErrorCodeToInt(swift::ErrorCode::TOKEN_INVALID),
                       "token invalid or missing");
        return ::grpc::Status::OK;
    }
    if (request->file_id().empty()) {
        SetResponseFail(response, swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM),
                       swift::ErrorCodeToString(swift::ErrorCode::INVALID_PARAM));
        return ::grpc::Status::OK;
    }
    bool ok = service_->DeleteFile(request->file_id(), user_id);
    if (ok) {
        SetResponseOk(response);
    } else {
        response->set_code(swift::ErrorCodeToInt(swift::ErrorCode::FILE_NOT_FOUND));
        response->set_message(swift::ErrorCodeToString(swift::ErrorCode::FILE_NOT_FOUND));
    }
    return ::grpc::Status::OK;
}

::grpc::Status FileHandler::GetUploadToken(::grpc::ServerContext* context,
                                            const ::swift::file::GetUploadTokenRequest* request,
                                            ::swift::file::UploadTokenResponse* response) {
    std::string user_id = GetUserId(context, request->user_id());
    if (user_id.empty()) {
        SetResponseFail(response, swift::ErrorCodeToInt(swift::ErrorCode::TOKEN_INVALID),
                       "token invalid or missing");
        return ::grpc::Status::OK;
    }
    if (request->file_name().empty()) {
        SetResponseFail(response, swift::ErrorCodeToInt(swift::ErrorCode::INVALID_PARAM),
                       swift::ErrorCodeToString(swift::ErrorCode::INVALID_PARAM));
        return ::grpc::Status::OK;
    }
    auto result = service_->GetUploadToken(user_id, request->file_name(), request->file_size());
    if (result.success) {
        SetResponseOk(response);
        response->set_upload_token(result.upload_token);
        response->set_upload_url(result.upload_url);
        response->set_expire_at(result.expire_at);
    } else {
        SetResponseFail(response, result.error_code, result.error);
    }
    return ::grpc::Status::OK;
}

// -----------------------------------------------------------------------------
// HttpDownloadHandler
// -----------------------------------------------------------------------------

HttpDownloadHandler::HttpDownloadHandler(std::shared_ptr<FileServiceCore> service)
    : service_(std::move(service)) {}

HttpDownloadHandler::~HttpDownloadHandler() = default;

bool HttpDownloadHandler::Serve(const std::string& file_id, int64_t range_start, int64_t range_end,
                                 std::vector<char>& data, std::string& content_type,
                                 std::string& file_name, int64_t& total_size, bool& is_partial) {
    is_partial = false;
    if (range_start >= 0 && range_end >= 0 && range_end >= range_start) {
        int64_t length = range_end - range_start + 1;
        bool ok = service_->ReadFileRange(file_id, range_start, length, data, content_type,
                                         file_name, total_size);
        if (ok)
            is_partial = true;
        return ok;
    }
    if (!service_->ReadFile(file_id, data, content_type, file_name))
        return false;
    total_size = static_cast<int64_t>(data.size());
    return true;
}

}  // namespace swift::file
