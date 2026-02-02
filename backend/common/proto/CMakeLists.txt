cmake_minimum_required(VERSION 3.16)
project(common_proto)

find_package(Protobuf REQUIRED)

set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/common.proto
)

# 生成目录（生成的 .pb.h/.pb.cc 放在 proto 目录下）
set(PROTO_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PROTO_SRCS)
set(PROTO_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    
    set(PROTO_SRC ${PROTO_GEN_DIR}/${PROTO_NAME}.pb.cc)
    set(PROTO_HDR ${PROTO_GEN_DIR}/${PROTO_NAME}.pb.h)
    
    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${PROTO_HDR})
    
    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --cpp_out=${PROTO_GEN_DIR}
             -I${CMAKE_CURRENT_SOURCE_DIR}
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating ${PROTO_NAME}.pb.h/cc"
    )
endforeach()

add_library(${PROJECT_NAME} STATIC ${PROTO_SRCS} ${PROTO_HDRS})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Protobuf_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${Protobuf_LIBRARIES}
)
