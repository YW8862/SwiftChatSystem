cmake_minimum_required(VERSION 3.16)
project(onlinesvr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROCKSDB_LIBS rocksdb pthread dl z snappy lz4 bz2)

set(SOURCES
    cmd/main.cpp
    internal/store/session_store.cpp
    internal/service/online_service.cpp
    internal/handler/online_handler.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
    ${CMAKE_CURRENT_SOURCE_DIR}/proto
    ${CMAKE_SOURCE_DIR}/backend/common/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    swift_common
    swift_proto
    ${ROCKSDB_LIBS}
)
