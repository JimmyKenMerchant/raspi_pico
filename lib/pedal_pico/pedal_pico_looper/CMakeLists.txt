# Turn on to show actual commands on "make" (only for related commands in this file).
set(CMAKE_VERBOSE_MAKEFILE off)

set(target_name pedal_pico_looper)

# With No Setting CMAKE_MODULE_PATH
include(${CMAKE_SOURCE_DIR}/cmake_module/pedal_pico.cmake)

# With Setting CMAKE_MODULE_PATH
#include(pedal_pico)

target_link_options(${target_name}
    PUBLIC "-T${CMAKE_CURRENT_SOURCE_DIR}/${target_name}_append.ld"
    #PUBLIC "-v"
)
