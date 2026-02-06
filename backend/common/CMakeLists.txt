cmake_minimum_required(VERSION 3.16)
project(swift_common)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 OpenSSL、nlohmann_json（必需）
find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")

# 编译 Proto 文件
add_subdirectory(proto)

# 源文件
set(SOURCES
    src/utils.cpp
    src/config.cpp
    src/config_loader.cpp
    src/jwt_helper.cpp
    src/async_write.cpp
)

# 公共库
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接依赖
target_link_libraries(${PROJECT_NAME} PUBLIC
    asynclogger::asynclogger
    OpenSSL::SSL
    OpenSSL::Crypto
)
# nlohmann_json 由 vcpkg/工具链提供 include，无需显式 link

# 测试程序
add_executable(test_utils tests/test_utils.cpp)
target_link_libraries(test_utils PRIVATE ${PROJECT_NAME})

# ============================================================================
# gRPC 鉴权辅助库（供 FriendSvr、ChatSvr 等从 metadata 解析 JWT 得到 user_id）
# ============================================================================
find_library(GRPC_CPP_LIBRARY grpc++ REQUIRED)
add_library(swift_grpc_auth STATIC src/grpc_auth.cpp)
target_include_directories(swift_grpc_auth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(swift_grpc_auth PUBLIC ${PROJECT_NAME} ${GRPC_CPP_LIBRARY})
