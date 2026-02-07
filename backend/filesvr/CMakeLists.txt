cmake_minimum_required(VERSION 3.16)
project(filesvr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROCKSDB_LIBS
    rocksdb
    pthread
    dl
    z
    snappy
    lz4
    bz2
)

set(SOURCES
    cmd/main.cpp
    internal/config/config.cpp
    internal/handler/file_handler.cpp
    internal/store/file_store.cpp
    internal/service/file_service.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
    ${CMAKE_SOURCE_DIR}/backend/common/include
    $<TARGET_PROPERTY:swift_proto,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    swift_common
    swift_grpc_auth
    swift_proto
    ${ROCKSDB_LIBS}
)

# ============================================================================
# 单元测试
# ============================================================================
option(BUILD_FILESVR_TESTS "Build filesvr tests" ON)

if(BUILD_FILESVR_TESTS)
    enable_testing()

    # FileStore 测试
    add_executable(file_store_test
        internal/store/file_store.cpp
        internal/store/file_store_test.cpp
    )
    target_include_directories(file_store_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
        ${CMAKE_SOURCE_DIR}/backend/common/include
    )
    target_link_libraries(file_store_test PRIVATE
        gtest
        gtest_main
        ${ROCKSDB_LIBS}
    )
    add_test(NAME file_store_test COMMAND file_store_test)

    # FileServiceCore 测试
    add_executable(file_service_test
        internal/store/file_store.cpp
        internal/service/file_service.cpp
        internal/service/file_service_test.cpp
    )
    target_include_directories(file_service_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
        ${CMAKE_SOURCE_DIR}/backend/common/include
    )
    target_link_libraries(file_service_test PRIVATE
        gtest
        gtest_main
        swift_common
        ${ROCKSDB_LIBS}
    )
    add_test(NAME file_service_test COMMAND file_service_test)
endif()
