/**
 * Written Codes:
 * Copyright 2021 Kenta Ishii
 * License: 3-Clause BSD License
 * SPDX Short Identifier: BSD-3-Clause
 *
 * Raspberry Pi Pico SDK:
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Standards
#include <stdio.h>
#include <stdio.h>
// Dependancies
#include "pico/stdlib.h"
// raspi_pico/include
#include "macros_pico.h"

/**
 * __flash_binary_end can be watched in qspi_flash.elf.map.
 * __flash_binary_end is also used in "pido-sdk/src/rp2_common/pico_standard_link/binary_info.c".
 */
#if !PICO_NO_FLASH
    #ifndef PICO_NO_BI_BINARY_SIZE
        extern char __flash_binary_end;
    #else
        #error PICO_NO_BI_BINARY_SIZE is defined.
    #endif
#else
    #error PICO_NO_FLASH is true.
#endif

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // Wait for Rediness of USB for Messages
    // Binary End
    printf("@main 1 - &__flash_binary_end %0x\n", (intptr_t)&__flash_binary_end);
    while (true) {
        tight_loop_contents();
    }
}
