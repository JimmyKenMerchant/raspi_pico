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
#include <stdlib.h>
// Dependancies
#include "pico/stdlib.h"
// raspi_pico/include
#include "macros_pico.h"
#include "util_pedal_pico.h"

/**
 * Caution! This program tries to erase and write data to the on-board flash memory.
 * This program turns off XIP, thus instruction code have to be stored at SRAM.
 */
#if !PICO_COPY_TO_RAM
    #error PICO_COPY_TO_RAM is false.
#endif

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

#define QPSI_FLASH_SW_1_GPIO 14

uint32 qspi_flash_debug_time;

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // Wait for Rediness of USB for Messages
    // Make Pull Up Switch
    uint32 gpio_mask = 0b1<< QPSI_FLASH_SW_1_GPIO;
    gpio_init_mask(gpio_mask);
    gpio_set_dir_masked(gpio_mask, 0x00000000);
    gpio_pull_up(QPSI_FLASH_SW_1_GPIO);
    // Binary End
    printf("@main 1 - &__flash_binary_end %0x\n", (intptr_t)&__flash_binary_end);
    printf("@main 2 - xip_ctrl_hw->ctrl %0x\n", xip_ctrl_hw->ctrl);
    util_pedal_pico_xip_turn_off();
    __dsb();
    printf("@main 3 - xip_ctrl_hw->ctrl %0x\n", xip_ctrl_hw->ctrl);
    uchar8* binary_end = (uchar8*)&__flash_binary_end;
    printf("@main 4 - binary_end %0x\n", binary_end);
    printf("@main 4 - *binary_end %0x\n", *binary_end);
    binary_end -= 1;
    printf("@main 4 - binary_end %0x\n", binary_end);
    printf("@main 4 - *binary_end %0x\n", *binary_end);
    binary_end += 1;
    if ((uint32)binary_end % FLASH_SECTOR_SIZE) binary_end = (uchar8*)((uint32)(binary_end + FLASH_SECTOR_SIZE) & ~(0xFFFFFFFF & (FLASH_SECTOR_SIZE - 1))); // 4096-byte (1024 Words) Aligned Sector (256-byte Aligned Page)
    uint16* free_start = (uint16*)binary_end;
    printf("@main 6 - free_start %0x\n", free_start);
    printf("@main 7 - *free_start %0x\n", *free_start);
    int32 free_start_offset = (uint32)free_start - XIP_BASE; // XIP_BASE = 0x10000000
    printf("@main 8 - free_start_offset %0x\n", free_start_offset);
    uint16 array_int[FLASH_SECTOR_SIZE >> 1];
    qspi_flash_debug_time = 0;
    uint16 increment = 1;
    while (true) {
        uint32 status_sw = gpio_get_all() & (0b1 << QPSI_FLASH_SW_1_GPIO);
        printf("@main 9 - xip_ctrl_hw->ctrl %0x\n", xip_ctrl_hw->ctrl);
        for (uint32 i = 0; i < FLASH_SECTOR_SIZE >> 1; i++) {
          //array_int[i] = rand();
          array_int[i] = increment;
        }
        uint32 from_time = time_us_32();
        if (status_sw) { //  // Erase and Write Data If Switch Off
            util_pedal_pico_flash_write(free_start_offset, (uchar8*)array_int, FLASH_SECTOR_SIZE);
        }
        qspi_flash_debug_time = time_us_32() - from_time;
        printf("@main 10 - qspi_flash_debug_time %d\n", qspi_flash_debug_time);
        for (uint32 i = 0; i < FLASH_PAGE_SIZE / 4; i++) { // 256-byte Aligned Page
          printf("free_start %d %0x\n", i, free_start[i]);
        }
        increment++;
        sleep_ms(1000);
        __dsb();
    }
}