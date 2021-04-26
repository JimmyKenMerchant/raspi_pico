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
