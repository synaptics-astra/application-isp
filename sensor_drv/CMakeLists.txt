cmake_minimum_required(VERSION 3.2.0)

# define module name
set (module sensor_drv)

include( sensor_drv_config.cmake )

add_definitions(-DLINUX -DDOLPHIN)
#set (MY_FLAGS "-Wl,-z,noexecstack -pie -funwind-tables")
#set (MY_CXX_FLAGS "-fvisibility=hidden -Wl,-z,noexecstack -pie -funwind-tables")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MY_FLAGS}")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MY_CXX_FLAGS}")


set(sensor_drv_source "")
set(sensor_drv_include "")
foreach(drv_name ${SENSOR_DRV_LIST})
    message("support sensor drv: ${drv_name}")
    list(APPEND sensor_drv_source ${drv_name}/source/${drv_name}.c)
    list(APPEND sensor_drv_include ${drv_name}/include_priv)
endforeach(drv_name)


# we want to compile all .c files as default
file(GLOB libsources
    sensor_drv.c
    ${sensor_drv_source}
    )

# define include paths
include_directories(
    include
	include/isi/
	include/isi/isi
    ${sensor_drv_include}
    )

# module specific defines
###add_definitions(-Wno-error=unused-function)
# add lib to build env

# add lib to build env
add_library(${module}_shared SHARED ${libsources})

SET_TARGET_PROPERTIES(${module}_shared PROPERTIES OUTPUT_NAME     ${module})
SET_TARGET_PROPERTIES(${module}_shared PROPERTIES LINK_FLAGS      "-shared -fPIC")
SET_TARGET_PROPERTIES(${module}_shared PROPERTIES FRAMEWORK       TRUE PUBLIC_HEADER "${pub_headers}")
install(TARGETS ${module}_shared
        ARCHIVE         DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        )
