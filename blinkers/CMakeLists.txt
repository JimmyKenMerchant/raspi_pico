set(name_executable blinkers)

add_executable(${name_executable}
    main.c
)

target_link_libraries(${name_executable} pico_stdlib)

# Beside elf, Create map/bin/hex/uf2 files.
pico_add_extra_outputs(${name_executable})

if (TARGET tinyusb_device)
    pico_enable_stdio_usb(${name_executable} 1)
elseif (PICO_ON_DEVICE)
    message(CAUTION "You can't monitor outputting messages via USB.")
endif()

# "printf" Messages via UART
#pico_enable_stdio_uart(${name_executable} 0)
