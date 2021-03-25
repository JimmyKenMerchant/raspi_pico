cmake_minimum_required(VERSION 3.13)
# Set where is pico-sdk from BUILD FOLDER.
set(PICO_SDK_PATH ../../pico-sdk)

include(${CMAKE_SOURCE_DIR}/pico_sdk_import.cmake)
include_directories(${CMAKE_SOURCE_DIR}/include)

project(kenta_projects C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(lib)
add_subdirectory(blinkers)
add_subdirectory(twin_dimmers)
