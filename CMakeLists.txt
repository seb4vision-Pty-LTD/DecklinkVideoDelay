cmake_minimum_required(VERSION 3.20)
project(DeckLinkDelayApp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0 )

include_directories(
    ${GST_INCLUDE_DIRS}
    include
)


add_executable(DeckLinkDelayApp
    src/main.cpp
    include/MainWindow.h
    src/MainWindow.cpp
)

target_link_directories(DeckLinkDelayApp PRIVATE
    ${GST_LIBRARY_DIRS}
)

target_link_libraries(DeckLinkDelayApp PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    ${GST_LIBRARIES}
    gstvideo-1.0
    gstaudio-1.0
)