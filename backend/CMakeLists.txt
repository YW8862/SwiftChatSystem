cmake_minimum_required(VERSION 3.16)
project(swift_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 公共库
add_subdirectory(common)

# 后端服务
add_subdirectory(authsvr)
add_subdirectory(onlinesvr)
add_subdirectory(gatesvr)
add_subdirectory(zonesvr)
add_subdirectory(friendsvr)
add_subdirectory(chatsvr)
add_subdirectory(filesvr)
