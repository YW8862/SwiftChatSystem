cmake_minimum_required(VERSION 3.16)
project(SwiftChat)

set(CMAKE_CXX_STANDARD 17)

# Qt 配置
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Network 
    WebSockets 
    REQUIRED
)

# 自动收集 UI 目录，避免手工维护文件列表导致漏改
file(GLOB UI_SOURCES CONFIGURE_DEPENDS
    src/ui/*.cpp
)
file(GLOB UI_HEADERS CONFIGURE_DEPENDS
    src/ui/*.h
)

# 源文件
set(SOURCES
    src/main.cpp
    ${UI_SOURCES}
    src/network/websocket_client.cpp
    src/network/protocol_handler.cpp
    src/network/app_service.cpp
    src/models/user.cpp
    src/models/message.cpp
    src/models/conversation.cpp
    src/utils/settings.cpp
    src/utils/image_utils.cpp
    src/utils/client_logger.cpp
)

# 头文件
set(HEADERS
    ${UI_HEADERS}
    src/network/websocket_client.h
    src/network/protocol_handler.h
    src/network/app_service.h
    src/models/user.h
    src/models/message.h
    src/models/conversation.h
    src/utils/settings.h
    src/utils/image_utils.h
    src/utils/client_logger.h
    src/swift/log_helper.h
)

# 资源文件
set(RESOURCES
    resources/resources.qrc
)

# 可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::WebSockets
    client_proto
    asynclogger::asynclogger
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Windows 特定配置
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
