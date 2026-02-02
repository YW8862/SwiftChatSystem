#pragma once

#include <memory>

namespace swift::zone {

class ZoneService;

class ZoneHandler {
public:
    explicit ZoneHandler(std::shared_ptr<ZoneService> service);
    ~ZoneHandler();
    
    // gRPC 接口
    // UserOnline / UserOffline / RouteMessage / Broadcast / GetUserStatus ...
    
private:
    std::shared_ptr<ZoneService> service_;
};

}  // namespace swift::zone
