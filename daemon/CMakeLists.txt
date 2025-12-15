cmake_minimum_required(VERSION 3.20.0)

project(
  isp_media_server
  VERSION 1.0.0
  LANGUAGES C)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#set (MY_FLAGS "-Wl,-z,noexecstack -pie -funwind-tables")
#set (MY_CXX_FLAGS "-fvisibility=hidden -Wl,-z,noexecstack -pie -funwind-tables")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MY_FLAGS}")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MY_CXX_FLAGS}")


add_definitions(-Wall)
add_definitions(-fPIC)

file(GLOB SOURCES *.c )

link_directories(../lib)
add_link_options(-pthread)

add_executable(${PROJECT_NAME} ${SOURCES})

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin)

target_link_libraries(
    ${PROJECT_NAME}
    isp_driver
    sensor_drv
    t_database_c
    t_json_c
    t_common_c
    mc_media_device
)
