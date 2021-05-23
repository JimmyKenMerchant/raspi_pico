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

#define PEDAL_PICO_LOOPER_INDICATOR_LED_TOGGLE_COUNT_ON_ERASE_MAX 10000
#define PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE 2 // Half Word
#define PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX FLASH_SECTOR_SIZE // 4096 Half Words = 8192 Bytes
#define PEDAL_PICO_LOOPER_FLASH_OFFSET_INDEX_MAX 200 // PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * 200 = 1638400 Bytes
#define PEDAL_PICO_LOOPER_FLASH_INDEX_MAX PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_FLASH_OFFSET_INDEX_MAX
#define PEDAL_PICO_LOOPER_FOOT_SW_RESET_THRESHOLD 56250

/**
 * This library uses 1638400 bytes in Flash Memory. Pico has 2097152 (2M) bytes Flash Memory.
 * If the samplig rate is 28125Hz, the maximum recording time becomes as follows:
 * 1638400 bytes divided by (28125Hz * 2 bytes) equals 29.127 seconds.
 */

volatile util_pedal_pico* pedal_pico_looper;
volatile uchar8 pedal_pico_looper_indicator_led;
volatile uint32 pedal_pico_looper_indicator_led_bit;
volatile uint16 pedal_pico_looper_conversion_1;
volatile uint16 pedal_pico_looper_conversion_2;
volatile uint16 pedal_pico_looper_conversion_3;
volatile uint16 pedal_pico_looper_loss;
volatile uchar8* pedal_pico_looper_flash;
volatile uint32 pedal_pico_looper_flash_index;
volatile uint32 pedal_pico_looper_flash_upto;
volatile uint32 pedal_pico_looper_flash_offset;
volatile uint32 pedal_pico_looper_flash_reserve_offset;
volatile uint16 pedal_pico_looper_flash_offset_index;
volatile uint16 pedal_pico_looper_flash_offset_upto;
volatile int16* pedal_pico_looper_buffer_in_1;
volatile int16* pedal_pico_looper_buffer_in_2;
volatile int16* pedal_pico_looper_buffer_out_1;
volatile int16* pedal_pico_looper_buffer_out_2;
volatile uchar8 pedal_pico_looper_sw_mode;
volatile uint32 pedal_pico_looper_sw_count;
volatile uint32 pedal_pico_looper_led_toggle_count_on_erase;
/**
 * pedal_pico_looper_buffer_status:
 * Bit[0]: Double Buffer Select
 * Bit[1]: Back Buffer is Outstanding to Write
 * Bit[2]: Recording on Set, Playing on Clear
 * Bit[3]: Outstanding to Rewind
 * Bit[4]: Outstanding to Reset
 * Bit[5]: Outstanding to Erase
 * Bit[6]: Pending
 * Bit[7]: Order to Rewind
 */
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS 0b00000001
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS 0b00000010
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_RECORDING_BITS 0b00000100
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS 0b00001000
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_RESET_BITS 0b00010000
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_ERASE_BITS 0b00100000
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS 0b01000000
#define PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_ORDER_REWIND_BITS 0b10000000
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
        #error "PICO_NO_BI_BINARY_SIZE is defined."
    #endif
    extern char __pedal_pico_looper_flash; // From Additional Linker Script, "pedal_pico_looper_append.ld"
#else
    #error "PICO_NO_FLASH is true."
#endif

static uchar8 pedal_pico_looper_flash_reserve[FLASH_SECTOR_SIZE] __attribute__((section (".PEDAL_PICO_LOOPER.FLASH"))) = {
    _4_BIG(_1000(0x88)) _2_BIG(_47(0x88)) 0x88, 0x88
}; // One Sector = 4096 Bytes, Fill by 0x88

void pedal_pico_looper_set(uchar8 indicator_led_gpio);
void pedal_pico_looper_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode);
void pedal_pico_looper_flash_handler();
void pedal_pico_looper_free();

#ifdef __cplusplus
}
#endif

#endif
