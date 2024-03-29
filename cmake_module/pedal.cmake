# CMake Module for pedal/pedal_.*

target_compile_definitions(${target_name}
    PRIVATE PICO_MALLOC_PANIC=1
    PRIVATE PICO_DEBUG_MALLOC=0
)

# Binary Info
pico_set_program_name(${target_name}
    "Guitar Pedal - ${target_name}"
)

# Binary Info
pico_set_program_description(${target_name}
    "This is a guitar pedal with 3-Clause BSD License: Copyright 2021 Kenta Ishii."
)

# Binary Info
pico_set_program_version(${target_name}
    "v1.1"
)

# Binary Info
pico_set_program_url(${target_name}
    "ukulele.jimmykenmerchant.com"
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
