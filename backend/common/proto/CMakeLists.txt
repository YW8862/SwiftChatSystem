cmake_minimum_required(VERSION 3.16)
project(swift_proto)

# 查找 Protobuf 和 gRPC
# CMake 4.x 的 FindProtobuf 有时不设置 Protobuf_LIBRARIES（或缓存为 NOTFOUND），导致 REQUIRED 报错；先清缓存再 find，确保有值
unset(Protobuf_LIBRARIES CACHE)
find_library(Protobuf_LIBRARIES NAMES protobuf)
if(Protobuf_LIBRARIES)
  set(Protobuf_LIBRARIES "${Protobuf_LIBRARIES}" CACHE STRING "Protobuf library" FORCE)
endif()
find_package(Protobuf REQUIRED)

# 查找 gRPC 插件
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
find_library(GRPC_LIBRARY grpc++ REQUIRED)
find_library(GRPC_LIBRARY_GRPC grpc REQUIRED)

message(STATUS "Protobuf version: ${Protobuf_VERSION}")
if(Protobuf_INCLUDE_DIRS)
  message(STATUS "Protobuf include: ${Protobuf_INCLUDE_DIRS}")
elseif(TARGET protobuf::libprotobuf)
  message(STATUS "Protobuf: using target protobuf::libprotobuf")
endif()
message(STATUS "gRPC plugin: ${GRPC_CPP_PLUGIN}")

# Proto 文件目录 - 使用绝对路径
set(BACKEND_DIR ${CMAKE_SOURCE_DIR}/backend)
set(PROTO_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_OUTPUT_DIR})

# 定义 proto 文件列表
set(PROTO_FILES
    ${BACKEND_DIR}/common/proto/common.proto
    ${BACKEND_DIR}/authsvr/proto/auth.proto
    ${BACKEND_DIR}/chatsvr/proto/chat.proto
    ${BACKEND_DIR}/chatsvr/proto/group.proto
    ${BACKEND_DIR}/friendsvr/proto/friend.proto
    ${BACKEND_DIR}/filesvr/proto/file.proto
    ${BACKEND_DIR}/gatesvr/proto/gate.proto
    ${BACKEND_DIR}/onlinesvr/proto/online.proto
    ${BACKEND_DIR}/zonesvr/proto/zone.proto
)

# Proto 搜索路径
set(PROTO_PATHS
    ${BACKEND_DIR}/common/proto
    ${BACKEND_DIR}/authsvr/proto
    ${BACKEND_DIR}/chatsvr/proto
    ${BACKEND_DIR}/friendsvr/proto
    ${BACKEND_DIR}/filesvr/proto
    ${BACKEND_DIR}/gatesvr/proto
    ${BACKEND_DIR}/onlinesvr/proto
    ${BACKEND_DIR}/zonesvr/proto
)

# 构建 -I 参数
set(PROTO_PATH_ARGS)
foreach(PATH ${PROTO_PATHS})
    list(APPEND PROTO_PATH_ARGS --proto_path=${PATH})
endforeach()

# 生成的文件列表
set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    
    # Protobuf 生成的文件
    list(APPEND PROTO_SRCS ${PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.cc)
    list(APPEND PROTO_HDRS ${PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.h)
    
    # gRPC 生成的文件
    list(APPEND GRPC_SRCS ${PROTO_OUTPUT_DIR}/${PROTO_NAME}.grpc.pb.cc)
    list(APPEND GRPC_HDRS ${PROTO_OUTPUT_DIR}/${PROTO_NAME}.grpc.pb.h)
    
    # 添加生成规则
    add_custom_command(
        OUTPUT 
            ${PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.cc
            ${PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.h
            ${PROTO_OUTPUT_DIR}/${PROTO_NAME}.grpc.pb.cc
            ${PROTO_OUTPUT_DIR}/${PROTO_NAME}.grpc.pb.h
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            ${PROTO_PATH_ARGS}
            --cpp_out=${PROTO_OUTPUT_DIR}
            --grpc_out=${PROTO_OUTPUT_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating protobuf/grpc: ${PROTO_NAME}.proto"
    )
endforeach()

# 创建 proto 库
add_library(swift_proto STATIC
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_include_directories(swift_proto PUBLIC
    ${PROTO_OUTPUT_DIR}
)
if(TARGET protobuf::libprotobuf)
  target_link_libraries(swift_proto PUBLIC protobuf::libprotobuf ${GRPC_LIBRARY} ${GRPC_LIBRARY_GRPC})
else()
  target_include_directories(swift_proto PUBLIC ${Protobuf_INCLUDE_DIRS})
  target_link_libraries(swift_proto PUBLIC ${Protobuf_LIBRARIES} ${GRPC_LIBRARY} ${GRPC_LIBRARY_GRPC})
endif()

# 导出头文件目录
set(SWIFT_PROTO_INCLUDE_DIR ${PROTO_OUTPUT_DIR} PARENT_SCOPE)
