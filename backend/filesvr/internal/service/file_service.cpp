#include "file_service.h"

namespace swift::file {

FileService::FileService(std::shared_ptr<FileStore> store, const std::string& storage_path)
    : store_(std::move(store)), storage_path_(storage_path) {}

FileService::~FileService() = default;

// TODO: 实现

}  // namespace swift::file
