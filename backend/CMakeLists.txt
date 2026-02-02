cmake_minimum_required(VERSION 3.16)
project(swift_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 公共库
add_subdirectory(common)

# 先只构建 authsvr 测试日志
add_subdirectory(authsvr)

# 其他服务暂时注释，等日志跑通后再启用
# add_subdirectory(gatesvr)
# add_subdirectory(zonesvr)
# add_subdirectory(friendsvr)
# add_subdirectory(chatsvr)
# add_subdirectory(filesvr)
