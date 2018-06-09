
cmake_minimum_required(VERSION 2.8)

SET(APPLICATION_NAME iliDriver)

SET(GPIO_LIBRARY BCM)

if(GPIO_LIBRARY STREQUAL BCM)

SET(GPIO_LIB bcm2835)
add_definitions(-DUSING_BCM_LIB)
add_definitions(-DGPIO_INIT=bcm2835_init)
add_definitions(-DGPIO_SET_MODE=bcm2835_gpio_fsel)
add_definitions(-DGPIO_SET_PUD=bcm2835_gpio_set_pud)
add_definitions(-DGPIO_WRITE=bcm2835_gpio_write)
add_definitions(-DGPIO_SET_MULTI=bcm2835_gpio_set_multi)
add_definitions(-DGPIO_CLEAR_MULTI=bcm2835_gpio_clr_multi)
add_definitions(-DGPIO_OUTPUT=BCM2835_GPIO_FSEL_OUTP)
add_definitions(-DGPIO_INPUT=BCM2835_GPIO_FSEL_INPT)
add_definitions(-DGPIO_HIGH=HIGH)
add_definitions(-DGPIO_LOW=LOW)
add_definitions(-DGPIO_PUD_OFF=BCM2835_GPIO_PUD_OFF)

else(GPIO_LIBRARY STREQUAL BCM)

SET(GPIO_LIB pigpio)
add_definitions(-DUSING_PIGPIO_LIB)
add_definitions(-DGPIO_INIT=gpioInitialise)
add_definitions(-DGPIO_SET_MODE=gpioSetMode)
add_definitions(-DGPIO_SET_PUD=gpioSetPullUpDown)
add_definitions(-DGPIO_WRITE=gpioWrite)
add_definitions(-DGPIO_SET_MULTI=gpioWrite_Bits_0_31_Set)
add_definitions(-DGPIO_CLEAR_MULTI=gpioWrite_Bits_0_31_Clear)
add_definitions(-DGPIO_OUTPUT=PI_OUTPUT)
add_definitions(-DGPIO_INPUT=PI_INPUT)
add_definitions(-DGPIO_HIGH=PI_HIGH)
add_definitions(-DGPIO_LOW=PI_LOW)
add_definitions(-DGPIO_PUD_OFF=PI_PUD_OFF)

endif(GPIO_LIBRARY STREQUAL BCM)

#SET(CMAKE_C_FLAGS  "-Wall -g")
SET(CMAKE_C_FLAGS  "-Wall -O3")
SET(CMAKE_C_STANDARD 99)

#my header files
include_directories("include/driver/")

#source files for the project
file(GLOB_RECURSE SOURCES "src/driver/*.c")
file(GLOB_RECURSE SOURCES "src/*.c")



#SET(MANUAL_DRAW YES)

if(MANUAL_DRAW STREQUAL YES)

add_definitions(-DNO_FRAME_BUFFER)
SET(BCM_LIB)

else(MANUAL_DRAW STREQUAL YES)

SET(BCM_LIB bcm_host)
#headers for the frame buffer 
include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host)
include_directories(/opt/vc/include/interface/vmcs_host/linux)
link_directories(/opt/vc/lib)

endif(MANUAL_DRAW STREQUAL YES)



project (${APPLICATION_NAME})
add_executable(${APPLICATION_NAME} ${SOURCES})

#needed libraries
target_link_libraries(${APPLICATION_NAME} pthread ${GPIO_LIB} ${BCM_LIB} X11 Xrandr )


install(TARGETS ${APPLICATION_NAME} DESTINATION /usr/bin)
install(CODE "execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/install.sh ${CMAKE_CURRENT_SOURCE_DIR})")
