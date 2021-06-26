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
    pico_multicore
    hardware_pwm
    hardware_adc
    hardware_irq
    hardware_sync
    hardware_resets
    hardware_clocks
    hardware_flash
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

# Unique Definition for Debugging
target_compile_definitions(${target_name}
    PUBLIC UTIL_PEDAL_PICO_DEBUG=0
)

# GPIO Assignment
if("${PICO_BOARD}" STREQUAL "pico")
    target_compile_definitions(${target_name}
        PUBLIC UTIL_PEDAL_PICO_LED_1_GPIO=25
        PUBLIC UTIL_PEDAL_PICO_LED_2_MULTI_BIT_4_GPIO=8
        PUBLIC UTIL_PEDAL_PICO_SW_1_GPIO=10
        PUBLIC UTIL_PEDAL_PICO_SW_2_GPIO=9
        PUBLIC UTIL_PEDAL_PICO_PWM_1_GPIO=16
        PUBLIC UTIL_PEDAL_PICO_PWM_2_GPIO=17
        PUBLIC UTIL_PEDAL_PICO_MULTI_BIT_0_GPIO=11
        PUBLIC UTIL_PEDAL_PICO_MULTI_BIT_1_GPIO=15
        PUBLIC UTIL_PEDAL_PICO_MULTI_BIT_2_GPIO=12
        PUBLIC UTIL_PEDAL_PICO_MULTI_BIT_3_GPIO=14
    )
endif()

message("Target, ${target_name}, assigns GPIO pins for ${PICO_BOARD}.")
