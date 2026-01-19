cmake_minimum_required(VERSION 3.18)

project(
    chromecastplugin
    VERSION 0.1.0
    DESCRIPTION "Chromecast plugin for fooyin"
    HOMEPAGE_URL "https://github.com/yourusername/fooyin-chromecast"
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Fooyin REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

# Generate protobuf sources
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/cast_channel.proto)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})  # For generated protobuf headers
include_directories(${PROTOBUF_INCLUDE_DIRS})

create_fooyin_plugin(
    chromecastplugin
    DEPENDS Fooyin::Gui Fooyin::Core
    SOURCES src/chromecastplugin.cpp
            src/chromecastplugin.h
            src/core/device.cpp
            src/core/device.h
            src/core/discoverymanager.cpp
            src/core/discoverymanager.h
            src/core/castsocket.cpp
            src/core/castsocket.h
            src/core/castprotocol.cpp
            src/core/castprotocol.h
            src/core/communicationmanager.cpp
            src/core/communicationmanager.h
            src/core/httpserver.cpp
            src/core/httpserver.h
            src/core/transcodingmanager.cpp
            src/core/transcodingmanager.h
            src/core/chromecastoutput.cpp
            src/core/chromecastoutput.h
            src/core/chromecastlogging.cpp
            src/core/chromecastlogging.h
            src/integration/playbackintegrator.cpp
            src/integration/playbackintegrator.h
            src/integration/trackmetadata.cpp
            src/integration/trackmetadata.h
            src/ui/devicewidget.cpp
            src/ui/devicewidget.h
            src/ui/devicewidget.ui
            src/ui/chromecastsettingspage.cpp
            src/ui/chromecastsettingspage.h
            ${PROTO_SRCS}
            ${PROTO_HDRS}
)

# Link protobuf and OpenSSL
target_link_libraries(chromecastplugin PRIVATE
    ${PROTOBUF_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)
