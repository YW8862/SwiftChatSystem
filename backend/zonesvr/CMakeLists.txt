cmake_minimum_required(VERSION 3.16)
project(zonesvr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    cmd/main.cpp
    internal/config/config.cpp
    internal/interceptor/internal_secret_processor.cpp
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

target_link_libraries(${PROJECT_NAME} PRIVATE
    swift_common
    swift_proto
)
