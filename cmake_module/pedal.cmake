# CMake Module for pedal/pedal_.*

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
    util_pedal_pico
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

target_compile_definitions(${target_name}
    PRIVATE PICO_MALLOC_PANIC=1
    PRIVATE PICO_DEBUG_MALLOC=0
)

# Beside elf, Create map/bin/hex/uf2 files.
pico_add_extra_outputs(${target_name})

if (TARGET tinyusb_device)
    pico_enable_stdio_usb(${target_name} 0)
    message(NOTICE "You can't monitor outputting messages of ${target_name} via USB. Use UART.")
elseif (PICO_ON_DEVICE)
    message(NOTICE "You can't monitor outputting messages via USB because of No TinyUSB.")
endif()

# "printf" Messages via UART
pico_enable_stdio_uart(${target_name} 1)
