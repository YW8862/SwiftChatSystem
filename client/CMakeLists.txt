cmake_minimum_required(VERSION 3.16)
project(swift_client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: UTF-8 编码，消除 C4819 及中文字符串解析错误
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Proto 库（客户端协议）
add_subdirectory(proto)

# Qt 桌面客户端
add_subdirectory(desktop)
