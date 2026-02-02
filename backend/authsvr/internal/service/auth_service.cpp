#include "auth_service.h"

namespace swift::auth {

AuthService::AuthService(std::shared_ptr<UserStore> store)
    : store_(std::move(store)) {}

AuthService::~AuthService() = default;

// TODO: 实现各业务方法

}  // namespace swift::auth
