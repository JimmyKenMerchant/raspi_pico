# Turn on to show actual commands on "make" (only for related commands in this file).
set(CMAKE_VERBOSE_MAKEFILE off)

set(target_name pedal_reverb)

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

# Beside elf, Create map/bin/hex/uf2 files.
pico_add_extra_outputs(${target_name})

if (TARGET tinyusb_device)
    pico_enable_stdio_usb(${target_name} 1)
elseif (PICO_ON_DEVICE)
    message(NOTICE "You can't monitor outputting messages via USB because of No TinyUSB.")
endif()

# "printf" Messages via UART
#pico_enable_stdio_uart(${target_name} 0)
