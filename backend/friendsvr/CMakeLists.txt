cmake_minimum_required(VERSION 3.16)
project(friendsvr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# RocksDB 依赖（与 authsvr 一致）
set(ROCKSDB_LIBS rocksdb pthread dl z snappy lz4 bz2)

add_executable(${PROJECT_NAME}
    cmd/main.cpp
    internal/config/config.cpp
    internal/store/friend_store.cpp
    internal/service/friend_service.cpp
    internal/handler/friend_handler.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
    ${CMAKE_SOURCE_DIR}/backend/common/include
    $<TARGET_PROPERTY:swift_proto,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    swift_common
    swift_proto
    ${ROCKSDB_LIBS}
)

# ============================================================================
# 单元测试
# ============================================================================
option(BUILD_FRIENDSVR_TESTS "Build friendsvr tests" ON)

if(BUILD_FRIENDSVR_TESTS)
    enable_testing()
    add_executable(friend_store_test
        internal/store/friend_store.cpp
        internal/store/friend_store_test.cpp
    )
    target_include_directories(friend_store_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
    )
    target_link_libraries(friend_store_test PRIVATE
        gtest
        gtest_main
        ${ROCKSDB_LIBS}
        stdc++fs
    )
    add_test(NAME friend_store_test COMMAND friend_store_test)

    add_executable(friend_service_test
        internal/store/friend_store.cpp
        internal/service/friend_service.cpp
        internal/service/friend_service_test.cpp
    )
    target_include_directories(friend_service_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
        ${CMAKE_SOURCE_DIR}/backend/common/include
    )
    target_link_libraries(friend_service_test PRIVATE
        gtest
        gtest_main
        swift_common
        ${ROCKSDB_LIBS}
        stdc++fs
    )
    add_test(NAME friend_service_test COMMAND friend_service_test)
endif()
