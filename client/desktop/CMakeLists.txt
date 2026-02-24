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

# 源文件
set(SOURCES
    src/main.cpp
    src/ui/mainwindow.cpp
    src/ui/loginwindow.cpp
    src/ui/chatwidget.cpp
    src/ui/contactwidget.cpp
    src/ui/messageitem.cpp
    src/network/websocket_client.cpp
    src/network/protocol_handler.cpp
    src/models/user.cpp
    src/models/message.cpp
    src/models/conversation.cpp
    src/utils/settings.cpp
    src/utils/image_utils.cpp
)

# 头文件
set(HEADERS
    src/ui/mainwindow.h
    src/ui/loginwindow.h
    src/ui/chatwidget.h
    src/ui/contactwidget.h
    src/ui/messageitem.h
    src/network/websocket_client.h
    src/network/protocol_handler.h
    src/models/user.h
    src/models/message.h
    src/models/conversation.h
    src/utils/settings.h
    src/utils/image_utils.h
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
