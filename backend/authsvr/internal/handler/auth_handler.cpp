#include "auth_handler.h"
#include "../service/auth_service.h"

namespace swift::auth {

AuthHandler::AuthHandler(std::shared_ptr<AuthService> service)
    : service_(std::move(service)) {}

AuthHandler::~AuthHandler() = default;

// TODO: 实现各 gRPC 接口

}  // namespace swift::auth
