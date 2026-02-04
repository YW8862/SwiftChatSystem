cmake_minimum_required(VERSION 3.16)
project(authsvr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# RocksDB 依赖库
set(ROCKSDB_LIBS
    rocksdb
    pthread
    dl
    z
    snappy
    lz4
    bz2
)

# 源文件
set(SOURCES
    cmd/main.cpp
    internal/config/config.cpp
    internal/store/user_store.cpp
    internal/service/auth_service.cpp
    internal/handler/auth_handler.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# 包含目录（含 proto 生成头 auth.grpc.pb.h 等，来自 swift_proto 的生成目录）
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
    ${CMAKE_SOURCE_DIR}/backend/common/include
    $<TARGET_PROPERTY:swift_proto,INTERFACE_INCLUDE_DIRECTORIES>
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    swift_common
    swift_proto
    swift_grpc_auth
    ${ROCKSDB_LIBS}
)

# ============================================================================
# 单元测试
# ============================================================================
option(BUILD_AUTHSVR_TESTS "Build authsvr tests" ON)

if(BUILD_AUTHSVR_TESTS)
    enable_testing()
    
    # UserStore 测试
    add_executable(user_store_test
        internal/store/user_store.cpp
        internal/store/user_store_test.cpp
    )
    
    target_include_directories(user_store_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
    )
    
    target_link_libraries(user_store_test PRIVATE
        gtest
        gtest_main
        ${ROCKSDB_LIBS}
        stdc++fs  # std::filesystem
    )
    
    add_test(NAME user_store_test COMMAND user_store_test)
    
    # AuthService 测试
    add_executable(auth_service_test
        internal/store/user_store.cpp
        internal/service/auth_service.cpp
        internal/service/auth_service_test.cpp
    )
    
    target_include_directories(auth_service_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
        ${CMAKE_SOURCE_DIR}/backend/common/include
    )
    
    target_link_libraries(auth_service_test PRIVATE
        gtest
        gtest_main
        swift_common
        ${ROCKSDB_LIBS}
        stdc++fs
    )
    
    add_test(NAME auth_service_test COMMAND auth_service_test)
endif()
