#include "online_handler.h"
#include "../service/online_service.h"

namespace swift::online {

OnlineHandler::OnlineHandler(std::shared_ptr<OnlineService> service)
    : service_(std::move(service)) {}

OnlineHandler::~OnlineHandler() = default;

}  // namespace swift::online
