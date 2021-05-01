# CMake Module for lib/pedal_.*

add_library(${target_name} STATIC
    ${target_name}.c
)

# Add libraries by target names on "add_library" of themselves.
target_link_libraries(${target_name}
    pico_stdlib
    pico_multicore
    pico_divider
    hardware_pwm
    hardware_adc
    hardware_irq
    hardware_sync
    util_pedal_pico
)
