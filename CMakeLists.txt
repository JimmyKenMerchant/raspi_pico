cmake_minimum_required(VERSION 3.13)
# Set where is pico-sdk from BUILD FOLDER.
set(PICO_SDK_PATH ${CMAKE_SOURCE_DIR}/../pico-sdk)
set(PICO_COPY_TO_RAM 1)

# Default Board Type
if(NOT DEFINED PICO_BOARD)
    set(PICO_BOARD "pico")
endif()

include(${CMAKE_SOURCE_DIR}/pico_sdk_import.cmake)
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)
# Commented out because CMAKE_MODULE_PATH may/will be used in pico-sdk and others
# Omit ".cmake" extension in "include" if you use CMAKE_MODULE_PATH.
#set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_module)

project(kenta_projects C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(lib)
add_subdirectory(blinkers)
add_subdirectory(twin_dimmers)
add_subdirectory(servo)
add_subdirectory(func)
add_subdirectory(pedal)
add_subdirectory(qspi_flash)
