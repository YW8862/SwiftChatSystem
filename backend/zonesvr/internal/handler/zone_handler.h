#pragma once

#include <memory>

namespace swift::zone {

class ZoneService;

/**
 * 对外 API 层（Handler）
 * 直接实现 proto 定义的 ZoneService gRPC 接口，无独立 API 层。
 */
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
