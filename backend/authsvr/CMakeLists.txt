cmake_minimum_required(VERSION 3.16)
project(authsvr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源文件
set(SOURCES
    cmd/main.cpp
    # internal/config/config.cpp
    # internal/handler/auth_handler.cpp
    # internal/service/auth_service.cpp
    # internal/store/user_store.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    swift_common
    swift_proto
)
