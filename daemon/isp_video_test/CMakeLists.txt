cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set (module isp_video_test)

file(GLOB SOURCES "*.c" )

include_directories(.
    display
    display/wayland)

include_directories(
    /usr/include/drm
)

add_definitions(-DNUM_DISPLAY_BUFFERS=8)
link_directories(
    ${CMAKE_SOURCE_DIR}/display/build
)


add_executable(${module} ${SOURCES})

target_link_libraries(
    ${module}
    display
    drm
    pthread
    wayland-client
    wayland-server
    )

install(
  TARGETS ${module}
  RUNTIME DESTINATION bin)

add_subdirectory(display)
