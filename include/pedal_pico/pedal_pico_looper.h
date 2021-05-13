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

#ifndef _PEDAL_PICO_LOOPER_H
#define _PEDAL_PICO_LOOPER_H 1

// Standards
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// Dependancies
#include "pico/stdlib.h"
#include "pico/divider.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
// raspi_pico/include
#include "macros_pico.h"
#include "util_pedal_pico.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PEDAL_PICO_LOOPER_GAIN 1
#define PEDAL_PICO_LOOPER_LED_GPIO 12
#define PEDAL_PICO_LOOPER_LED_GPIO_BITS (0b1 << PEDAL_PICO_LOOPER_LED_GPIO)
#define PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE 2 // Half Word
#define PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX FLASH_SECTOR_SIZE // 4096 Half Words = 8192 Bytes
#define PEDAL_PICO_LOOPER_FLASH_OFFSET_INDEX_MAX 100 // PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * 100 = 819200 Bytes
#define PEDAL_PICO_LOOPER_FLASH_INDEX_MAX PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_FLASH_OFFSET_INDEX_MAX
#define PEDAL_PICO_LOOPER_FOOT_SW_RESET_THRESHOLD 56250

volatile util_pedal_pico* pedal_pico_looper;
volatile uint16 pedal_pico_looper_conversion_1;
volatile uint16 pedal_pico_looper_conversion_2;
volatile uint16 pedal_pico_looper_conversion_3;
volatile uint16 pedal_pico_looper_loss;
volatile int16* pedal_pico_looper_flash;
volatile uint32 pedal_pico_looper_flash_index;
volatile uint32 pedal_pico_looper_flash_upto;
volatile uint32 pedal_pico_looper_flash_offset;
volatile uint16 pedal_pico_looper_flash_offset_index;
volatile uint16 pedal_pico_looper_flash_offset_upto;
volatile int16* pedal_pico_looper_buffer_in_1;
volatile int16* pedal_pico_looper_buffer_in_2;
volatile int16* pedal_pico_looper_buffer_out_1;
volatile int16* pedal_pico_looper_buffer_out_2;
volatile uchar8 pedal_pico_looper_sw_mode;
volatile uint32 pedal_pico_looper_sw_count;
/**
 * pedal_pico_looper_buffer_status:
 * Bit[0]: Double Buffer Select
 * Bit[1]: Back Buffer is Outstanding to Write
 * Bit[2]: Recording on Set, Playing on Clear
 * Bit[3]: Outstanding to Rewind
 * Bit[4]: Outstanding to Erase
 * Bit[5]: Pending after Erase
 */
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS 0b00000001
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS 0b00000010
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_RECORDING_BITS 0b00000100
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS 0b00001000
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_ERASE_BITS 0b00010000
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS 0b00100000
volatile uchar8 pedal_pico_looper_buffer_status;
volatile int32* pedal_pico_looper_table_pdf_1;

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

void pedal_pico_looper_set();
void pedal_pico_looper_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode);
void pedal_pico_looper_flash_handler();
void pedal_pico_looper_free();

#ifdef __cplusplus
}
#endif

#endif