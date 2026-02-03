cmake_minimum_required(VERSION 3.16)
project(chatsvr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROCKSDB_LIBS rocksdb pthread dl z snappy lz4 bz2)

add_executable(${PROJECT_NAME}
    cmd/main.cpp
    internal/config/config.cpp
    internal/store/group_store.cpp
    internal/service/group_service.cpp
    internal/handler/group_handler.cpp
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
