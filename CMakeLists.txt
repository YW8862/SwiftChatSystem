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
# 仅构建客户端（适用于 Windows 平台，无需后端依赖）
option(SWIFT_BUILD_CLIENT_ONLY "Build only Qt client (no backend, for Windows)" OFF)

if(NOT SWIFT_BUILD_CLIENT_ONLY)
    add_subdirectory(backend)
endif()

# Qt 客户端（可选，需安装 Qt5 后启用）
option(SWIFT_BUILD_CLIENT "Build Qt desktop client" OFF)
if(SWIFT_BUILD_CLIENT OR SWIFT_BUILD_CLIENT_ONLY)
    add_subdirectory(client)
endif()
