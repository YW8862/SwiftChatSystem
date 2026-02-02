cmake_minimum_required(VERSION 3.16)
project(authsvr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 暂时只编译 main.cpp 测试日志
add_executable(${PROJECT_NAME}
    cmd/main.cpp
)

# 链接公共库（包含 asynclogger）
target_link_libraries(${PROJECT_NAME} PRIVATE
    swift_common
)

# 后续启用 gRPC 和 Proto 时取消注释
# find_package(Protobuf REQUIRED)
# find_package(gRPC CONFIG REQUIRED)
# ... proto 编译规则 ...
