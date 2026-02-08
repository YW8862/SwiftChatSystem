cmake_minimum_required(VERSION 3.16)
project(zonesvr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# hiredis 可选：有则启用 RedisSessionStore 完整实现，无则使用占位实现（仍可编译运行，仅 session_store_type=redis 时需安装）
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY hiredis)
if(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARY)
  set(ZONESVR_USE_HIREDIS 1)
endif()

set(SOURCES
    cmd/main.cpp
    internal/config/config.cpp
    internal/handler/zone_handler.cpp
    internal/interceptor/internal_secret_processor.cpp
    internal/service/zone_service.cpp
    internal/store/session_store.cpp
    internal/rpc/rpc_client_base.cpp
    internal/rpc/auth_rpc_client.cpp
    internal/rpc/online_rpc_client.cpp
    internal/rpc/chat_rpc_client.cpp
    internal/rpc/friend_rpc_client.cpp
    internal/rpc/group_rpc_client.cpp
    internal/rpc/file_rpc_client.cpp
    internal/rpc/gate_rpc_client.cpp
    internal/system/auth_system.cpp
    internal/system/chat_system.cpp
    internal/system/friend_system.cpp
    internal/system/group_system.cpp
    internal/system/file_system.cpp
    internal/system/system_manager.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
    ${CMAKE_SOURCE_DIR}/backend/common/include
    $<TARGET_PROPERTY:swift_proto,INTERFACE_INCLUDE_DIRECTORIES>
)
if(ZONESVR_USE_HIREDIS)
  target_include_directories(${PROJECT_NAME} PRIVATE ${HIREDIS_INCLUDE_DIR})
  target_compile_definitions(${PROJECT_NAME} PRIVATE ZONESVR_USE_HIREDIS=1)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${HIREDIS_LIBRARY})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    swift_common
    swift_proto
)
