cmake_minimum_required(VERSION 3.20)

project(gstrecognizer)

set(CMAKE_CXX_STANDARD 17)

add_library(gstrecognizer 
    SHARED
    src/gstrecognizer.cpp
    src/gstrecognizer.h
    src/gstdetectioninfo.h
    src/gstdetectioninfo.cpp
    src/recognizer.cpp)

find_package( OpenCV REQUIRED )
find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=1.4)

target_link_libraries(gstrecognizer PRIVATE 
    PkgConfig::gstreamer
    PkgConfig::gstreamer-video
    ${OpenCV_LIBS}
)

target_include_directories(gstrecognizer PRIVATE 
    ${GST_INCLUDE_DIRS}
)

set_target_properties(gstrecognizer PROPERTIES CXX_STANDARD 17)
set_target_properties(gstrecognizer 
    PROPERTIES OUTPUT_NAME "recognizer")