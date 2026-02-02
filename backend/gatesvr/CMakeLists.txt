cmake_minimum_required(VERSION 3.16)
project(gatesvr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    cmd/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    swift_common
    swift_proto
)
