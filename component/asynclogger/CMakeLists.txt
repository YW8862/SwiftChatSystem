cmake_minimum_required(VERSION 3.14)
project(AsyncLogger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# 构建选项
# ============================================================================
option(ASYNCLOGGER_BUILD_TESTS "Build test examples" ON)
option(ASYNCLOGGER_BUILD_SHARED "Build shared library instead of static" OFF)
option(ASYNCLOGGER_INSTALL "Enable install target" ON)

# ============================================================================
# 源文件
# ============================================================================
set(ASYNCLOGGER_SOURCES
    src/async_logger.cpp
    src/ring_buffer.cpp
    src/backend_thread.cpp
    src/file_sink.cpp
    src/console_sink.cpp
    src/log_formatter.cpp
)

set(ASYNCLOGGER_HEADERS
    include/async_logger/async_logger.h
    include/async_logger/log_config.h
    include/async_logger/log_level.h
)

# ============================================================================
# 创建库
# ============================================================================
if(ASYNCLOGGER_BUILD_SHARED)
    add_library(asynclogger SHARED ${ASYNCLOGGER_SOURCES})
    set_target_properties(asynclogger PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
else()
    add_library(asynclogger STATIC ${ASYNCLOGGER_SOURCES})
endif()

# 设置库的别名（用于 add_subdirectory 方式引用）
add_library(asynclogger::asynclogger ALIAS asynclogger)

target_include_directories(asynclogger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接线程库
find_package(Threads REQUIRED)
target_link_libraries(asynclogger PUBLIC Threads::Threads)

# 编译选项
target_compile_options(asynclogger PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# 导出符号（Windows DLL）
if(WIN32 AND ASYNCLOGGER_BUILD_SHARED)
    target_compile_definitions(asynclogger PRIVATE ASYNCLOGGER_EXPORTS)
endif()

# ============================================================================
# 测试示例
# ============================================================================
if(ASYNCLOGGER_BUILD_TESTS)
    add_executable(test_logger tests/test_logger.cpp)
    target_link_libraries(test_logger PRIVATE asynclogger)
    
    add_executable(test_performance tests/test_performance.cpp)
    target_link_libraries(test_performance PRIVATE asynclogger)
endif()

# ============================================================================
# 安装配置
# ============================================================================
if(ASYNCLOGGER_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    
    # 安装库文件
    install(TARGETS asynclogger
        EXPORT asynclogger-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    
    # 安装头文件
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    
    # 生成并安装 CMake 配置文件
    install(EXPORT asynclogger-targets
        FILE asynclogger-targets.cmake
        NAMESPACE asynclogger::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/asynclogger
    )
    
    # 生成版本文件
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/asynclogger-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    
    # 配置文件
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/asynclogger-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/asynclogger-config.cmake"
        @ONLY
    )
    
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/asynclogger-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/asynclogger-config-version.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/asynclogger
    )
endif()

# ============================================================================
# 打印配置信息
# ============================================================================
message(STATUS "")
message(STATUS "AsyncLogger Configuration:")
message(STATUS "  Version:       ${PROJECT_VERSION}")
message(STATUS "  Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  Library type:  ${ASYNCLOGGER_BUILD_SHARED}")
message(STATUS "  Build tests:   ${ASYNCLOGGER_BUILD_TESTS}")
message(STATUS "  Install:       ${ASYNCLOGGER_INSTALL}")
message(STATUS "")
