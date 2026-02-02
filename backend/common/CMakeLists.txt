cmake_minimum_required(VERSION 3.16)
project(swift_common)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 OpenSSL（必需）
find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")

# 编译 Proto 文件
add_subdirectory(proto)

# 源文件
set(SOURCES
    src/utils.cpp
    src/config.cpp
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

# 测试程序
add_executable(test_utils tests/test_utils.cpp)
target_link_libraries(test_utils PRIVATE ${PROJECT_NAME})
