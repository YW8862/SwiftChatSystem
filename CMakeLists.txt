cmake_minimum_required(VERSION 3.16)
project(SwiftChatSystem VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# 组件库（项目内部依赖）
# ============================================================================

# AsyncLogger 异步日志库
set(ASYNCLOGGER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASYNCLOGGER_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(component/asynclogger)

# ============================================================================
# 子项目
# ============================================================================
add_subdirectory(backend)
# add_subdirectory(client)  # Qt 客户端，需安装 Qt5 后取消注释
