# Turn on to show actual commands on "make" (only for related commands in this file).
set(CMAKE_VERBOSE_MAKEFILE off)

set(target_name pedal_looper)

add_executable(${target_name}
    ${target_name}.c
)

# Add libraries by target names on "add_library" of themselves.
target_link_libraries(${target_name}
    util_pedal_pico
    pedal_pico_looper
)

# With No Setting CMAKE_MODULE_PATH
include(${CMAKE_SOURCE_DIR}/cmake_module/pedal.cmake)

# With Setting CMAKE_MODULE_PATH
#include(pedal)

# GPIO Assignment
if("${PICO_BOARD}" STREQUAL "pico")
    target_compile_definitions(${target_name}
        PUBLIC PEDAL_LOOPER_LED_GPIO=25
    )
endif()

message("Target, ${target_name}, assigns GPIO pins for ${PICO_BOARD}.")
