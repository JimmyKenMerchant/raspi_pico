# Turn on to show actual commands on "make" (only for related commands in this file).
set(CMAKE_VERBOSE_MAKEFILE off)

set(target_name util_pedal_pico)

add_library(${target_name} STATIC
    ${target_name}.c
)

# Add libraries by target names on "add_library" of themselves.
target_link_libraries(${target_name}
    pico_stdlib
    pico_divider
    hardware_pwm
    hardware_adc
    hardware_irq
    hardware_sync
    hardware_resets
    hardware_clocks
)

# Extension for Number Table
add_custom_target(${target_name}_ex_header
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ./${target_name}_ex_make_header.py
    COMMAND cp ${target_name}_ex.h ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND rm ${target_name}_ex.h
)

add_dependencies(${target_name}
    ${target_name}_ex_header
)

target_include_directories(${target_name}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
