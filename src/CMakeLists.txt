# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

set(PICO_SDK_PATH "/home/wang/pico2/pico-sdk")
#"wang"need to change to your user name!

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(cube_robot C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(cube_robot cube_robot.c color_detect.c prun_table.c solve.c OLED.c OLED_Data.c)

pico_set_program_name(cube_robot "cube_robot")
pico_set_program_version(cube_robot "0.1")

pico_enable_stdio_uart(cube_robot 0)
pico_enable_stdio_usb(cube_robot 1)

# Add the standard library to the build
target_link_libraries(cube_robot pico_stdlib pico_multicore)

pico_add_extra_outputs(cube_robot)

