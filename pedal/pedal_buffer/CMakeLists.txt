# Turn on to show actual commands on "make" (only for related commands in this file).
set(CMAKE_VERBOSE_MAKEFILE off)

set(target_name pedal_buffer)

add_executable(${target_name}
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
)

add_custom_target(${target_name}_header
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ./${target_name}_make_header.py
    COMMAND cp ${target_name}.h ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND rm ${target_name}.h
)

add_dependencies(${target_name}
    ${target_name}_header
)

target_include_directories(${target_name} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

# Beside elf, Create map/bin/hex/uf2 files.
pico_add_extra_outputs(${target_name})

if (TARGET tinyusb_device)
    pico_enable_stdio_usb(${target_name} 1)
elseif (PICO_ON_DEVICE)
    message(NOTICE "You can't monitor outputting messages via USB because of No TinyUSB.")
endif()

# "printf" Messages via UART
pico_enable_stdio_uart(${target_name} 1)
