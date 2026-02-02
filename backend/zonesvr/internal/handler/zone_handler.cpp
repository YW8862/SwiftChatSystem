#include "zone_handler.h"

namespace swift::zone {

ZoneHandler::ZoneHandler(std::shared_ptr<ZoneService> service)
    : service_(std::move(service)) {}

ZoneHandler::~ZoneHandler() = default;

}  // namespace swift::zone
