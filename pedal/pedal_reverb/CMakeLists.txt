# Turn on to show actual commands on "make" (only for related commands in this file).
set(CMAKE_VERBOSE_MAKEFILE off)

set(target_name pedal_reverb)

add_executable(${target_name}
    ${target_name}.c
)

# Add libraries by target names on "add_library" of themselves.
target_link_libraries(${target_name}
    util_pedal_pico
    pedal_pico_reverb
)

# With No Setting CMAKE_MODULE_PATH
include(${CMAKE_SOURCE_DIR}/cmake_module/pedal.cmake)

# With Setting CMAKE_MODULE_PATH
#include(pedal)
