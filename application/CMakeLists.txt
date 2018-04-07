
cmake_minimum_required (VERSION 2.8)

SET(APPLICATION_NAME application)

SET(CMAKE_C_FLAGS  "-Wall" )
set(CMAKE_C_STANDARD 99)

include_directories("include/driver/")


file(GLOB_RECURSE SOURCES "src/driver/*.c")
file(GLOB_RECURSE SOURCES "src/*.c")

project (Application)
add_executable(${APPLICATION_NAME} ${SOURCES})
target_link_libraries(${APPLICATION_NAME} pthread pigpio)