set(target_name blinkers)

add_executable(${target_name}
    ${target_name}.c
)

target_link_libraries(${target_name} pico_stdlib sequencer_gpio_pico)

# Beside elf, Create map/bin/hex/uf2 files.
pico_add_extra_outputs(${target_name})

if (TARGET tinyusb_device)
    pico_enable_stdio_usb(${target_name} 1)
elseif (PICO_ON_DEVICE)
    message(NOTICE "You can't monitor outputting messages via USB because of No TinyUSB.")
endif()

# "printf" Messages via UART
#pico_enable_stdio_uart(${target_name} 0)
