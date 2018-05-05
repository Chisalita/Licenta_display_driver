
cmake_minimum_required (VERSION 2.8)

SET(APPLICATION_NAME application)

SET(CMAKE_C_FLAGS  "-Wall -g")
set(CMAKE_C_STANDARD 99)

#my header files
include_directories("include/driver/")

#source files for the project
file(GLOB_RECURSE SOURCES "src/driver/*.c")
file(GLOB_RECURSE SOURCES "src/*.c")


#headers for the frame buffer 
include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

link_directories(/opt/vc/lib)

project (${APPLICATION_NAME})
add_executable(${APPLICATION_NAME} ${SOURCES})

#needed libraries
target_link_libraries(${APPLICATION_NAME} pthread pigpio bcm_host X11 Xrandr)