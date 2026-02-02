cmake_minimum_required(VERSION 3.16)
project(swift_common)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 暂时注释 Proto（先跑通日志）
# add_subdirectory(proto)

# 源文件（暂时为空，只提供头文件）
# set(SOURCES
#     src/config.cpp
#     src/utils.cpp
# )

# 公共库（INTERFACE 库，只提供头文件路径）
add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 依赖 asynclogger
target_link_libraries(${PROJECT_NAME} INTERFACE
    asynclogger::asynclogger
)
