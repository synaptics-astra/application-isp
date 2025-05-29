cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_definitions(-Wall)
add_definitions(-fPIC)

set (module display)

file(GLOB DISPLAY_SOURCES "*.c" )
file(GLOB WAYLAND_SOURCES "wayland/*.c")

set(SOURCES ${DISPLAY_SOURCES} ${WAYLAND_SOURCES})

include_directories(.)
include_directories(
    wayland
    /usr/include
)

add_library(${module} SHARED ${SOURCES})

install(
    TARGETS ${module}
    LIBRARY DESTINATION lib)
