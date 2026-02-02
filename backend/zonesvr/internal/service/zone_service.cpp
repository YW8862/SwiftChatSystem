#include "zone_service.h"

namespace swift::zone {

ZoneService::ZoneService(std::shared_ptr<SessionStore> store)
    : store_(std::move(store)) {}

ZoneService::~ZoneService() = default;

// TODO: 实现各方法

}  // namespace swift::zone
