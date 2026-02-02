#include "file_handler.h"

namespace swift::file {

FileHandler::FileHandler(std::shared_ptr<FileService> service)
    : service_(std::move(service)) {}

FileHandler::~FileHandler() = default;

HttpDownloadHandler::HttpDownloadHandler(std::shared_ptr<FileService> service)
    : service_(std::move(service)) {}

HttpDownloadHandler::~HttpDownloadHandler() = default;

}  // namespace swift::file
