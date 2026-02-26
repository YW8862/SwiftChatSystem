cmake_minimum_required(VERSION 3.16)
project(swift_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GTest：未安装时关闭所有后端单元测试，避免编译报错 gtest/gtest.h not found
find_package(GTest QUIET)
if(GTest_FOUND)
  if(NOT TARGET gtest)
    add_library(gtest ALIAS GTest::gtest)
  endif()
  if(NOT TARGET gtest_main)
    add_library(gtest_main ALIAS GTest::main)
  endif()
else()
  set(BUILD_AUTHSVR_TESTS OFF CACHE BOOL "Build authsvr tests" FORCE)
  set(BUILD_FRIENDSVR_TESTS OFF CACHE BOOL "Build friendsvr tests" FORCE)
  set(BUILD_CHATSVR_TESTS OFF CACHE BOOL "Build chatsvr tests" FORCE)
  set(BUILD_FILESVR_TESTS OFF CACHE BOOL "Build filesvr tests" FORCE)
  message(STATUS "GTest not found; backend unit tests disabled. Install libgtest-dev to build tests.")
endif()

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
