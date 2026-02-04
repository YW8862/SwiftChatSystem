cmake_minimum_required(VERSION 3.16)
project(chatsvr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROCKSDB_LIBS rocksdb pthread dl z snappy lz4 bz2)

add_executable(${PROJECT_NAME}
    cmd/main.cpp
    internal/config/config.cpp
    internal/store/group_store.cpp
    internal/store/message_store.cpp
    internal/service/group_service.cpp
    internal/service/chat_service.cpp
    internal/handler/group_handler.cpp
    internal/handler/chat_handler.cpp
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
option(BUILD_CHATSVR_TESTS "Build chatsvr tests" ON)

if(BUILD_CHATSVR_TESTS)
    enable_testing()

    # MessageStore 测试
    add_executable(message_store_test
        internal/store/message_store.cpp
        internal/store/message_store_test.cpp
    )
    target_include_directories(message_store_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
        ${CMAKE_SOURCE_DIR}/backend/common/include
    )
    target_link_libraries(message_store_test PRIVATE
        gtest
        gtest_main
        ${ROCKSDB_LIBS}
        stdc++fs
    )
    add_test(NAME message_store_test COMMAND message_store_test)

    # ChatServiceCore 测试
    add_executable(chat_service_test
        internal/store/message_store.cpp
        internal/store/group_store.cpp
        internal/service/chat_service.cpp
        internal/service/chat_service_test.cpp
    )
    target_include_directories(chat_service_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
        ${CMAKE_SOURCE_DIR}/backend/common/include
    )
    target_link_libraries(chat_service_test PRIVATE
        gtest
        gtest_main
        swift_common
        ${ROCKSDB_LIBS}
        stdc++fs
    )
    add_test(NAME chat_service_test COMMAND chat_service_test)
endif()

