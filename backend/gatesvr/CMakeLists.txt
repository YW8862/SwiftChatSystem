cmake_minimum_required(VERSION 3.16)
project(gatesvr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Beast/Asio 使用 boost::system::error_code，需要 boost_system；部分环境 Boost 配置异常，使用变量避免 imported target 校验
find_package(Boost REQUIRED)

set(SOURCES
    cmd/main.cpp
    internal/config/config.cpp
    internal/handler/gate_internal_grpc_handler.cpp
    internal/handler/websocket_handler.cpp
    internal/rpc/zone_rpc_client.cpp
    internal/service/gate_service.cpp
    internal/websocket/ws_listener.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
    ${CMAKE_SOURCE_DIR}/backend/common/include
    ${Boost_INCLUDE_DIRS}
    $<TARGET_PROPERTY:swift_proto,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    swift_common
    swift_proto
    boost_system
)
