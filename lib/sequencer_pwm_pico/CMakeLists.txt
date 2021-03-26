# Turn on to show actual commands on "make" (only for related commands in this file).
set(CMAKE_VERBOSE_MAKEFILE off)

set(target_name sequencer_pwm_pico)

add_library(${target_name} STATIC
    ${target_name}.c
)

target_link_libraries(${target_name} pico_stdlib hardware_pwm)
