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

#include "pedal_pico/pedal_pico_looper.h"

void pedal_pico_looper_set() {
    if (! pedal_pico_looper) panic("pedal_pico_looper is not initialized.");
    pedal_pico_looper_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_looper_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_looper_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_looper_loss = 32 - (pedal_pico_looper_conversion_2 >> 7); // Make 5-bit Value (1-32)
    uchar8* binary_end = (uchar8*)&__flash_binary_end;
    if ((uint32)binary_end % FLASH_SECTOR_SIZE) binary_end = (uchar8*)((uint32)(binary_end + FLASH_SECTOR_SIZE) & ~(0xFFFFFFFF & (FLASH_SECTOR_SIZE - 1))); // 4096-byte (1024 Words) Aligned Sector (256-byte Aligned Page)
    pedal_pico_looper_flash = (int16*)binary_end;
    pedal_pico_looper_flash_index = 0;
    pedal_pico_looper_flash_upto = PEDAL_PICO_LOOPER_FLASH_INDEX_MAX;
    pedal_pico_looper_flash_offset = (uint32)binary_end - XIP_BASE;
    pedal_pico_looper_flash_offset_index = 0;
    pedal_pico_looper_flash_offset_upto = PEDAL_PICO_LOOPER_FLASH_OFFSET_INDEX_MAX;
    pedal_pico_looper_buffer_in_1 = (uint16*)calloc(PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX, sizeof(uint16));
    pedal_pico_looper_buffer_in_2 = (uint16*)calloc(PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX, sizeof(uint16));
    pedal_pico_looper_buffer_out_1 = (uint16*)calloc(PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX, sizeof(uint16));
    pedal_pico_looper_buffer_out_2 = (uint16*)calloc(PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX, sizeof(uint16));
    pedal_pico_looper_sw_mode = 0;
    pedal_pico_looper_sw_count = 0;
    pedal_pico_looper_buffer_status = PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_ERASE_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS;
    gpio_init(PEDAL_PICO_LOOPER_LED_GPIO);
    gpio_set_dir(PEDAL_PICO_LOOPER_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_PICO_LOOPER_LED_GPIO, 0);
}

void pedal_pico_looper_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    pedal_pico_looper_conversion_1 = conversion_1;
    if (abs(conversion_2 - pedal_pico_looper_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_looper_conversion_2 = conversion_2;
        pedal_pico_looper_loss = 32 - (pedal_pico_looper_conversion_2 >> 7); // Make 5-bit Value (1-32)
    }
    if (abs(conversion_3 - pedal_pico_looper_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_looper_conversion_3 = conversion_3;
    }
    if (sw_mode == 1) {
        if (! (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_ERASE_BITS)) {
            if (pedal_pico_looper_sw_mode != sw_mode) {
                pedal_pico_looper_sw_mode = sw_mode;
                if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_RECORDING_BITS) { // Among Recording
                    pedal_pico_looper_flash_upto = pedal_pico_looper_flash_index;
                    pedal_pico_looper_flash_offset_upto = (pedal_pico_looper_flash_upto % PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX) ? ((pedal_pico_looper_flash_upto / PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX) + 1) : (pedal_pico_looper_flash_upto / PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX);
                    pedal_pico_looper_buffer_status ^= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS;
                    pedal_pico_looper_buffer_status |= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS;
                }
                if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS) { // After Reset
                    pedal_pico_looper_buffer_status &= ~(PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS);
                }
                pedal_pico_looper_buffer_status ^= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_RECORDING_BITS; // Toggle Recording Status
                gpio_xor_mask(PEDAL_PICO_LOOPER_LED_GPIO_BITS);
            }
            pedal_pico_looper_sw_count++;
            if (pedal_pico_looper_sw_count >= PEDAL_PICO_LOOPER_FOOT_SW_RESET_THRESHOLD) {
                pedal_pico_looper_sw_count -= PEDAL_PICO_LOOPER_FOOT_SW_RESET_THRESHOLD;
                pedal_pico_looper_buffer_status = PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_ERASE_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS; // Exclusively
            }
        }
    } else {
        if (pedal_pico_looper_sw_mode != sw_mode) {
            pedal_pico_looper_sw_mode = sw_mode;
        }
        pedal_pico_looper_sw_count = 0;
    }
    int32 normalized_1 = (int32)pedal_pico_looper_conversion_1 - (int32)util_pedal_pico_adc_middle_moving_average;
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_looper_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, UTIL_PEDAL_PICO_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    normalized_1 = normalized_1 / pedal_pico_looper_loss;
    int32 recorded_1;
    int32 mixed_1;
    if (! (pedal_pico_looper_buffer_status & (PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS))) {
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS) {
            recorded_1 = pedal_pico_looper_buffer_in_2[pedal_pico_looper_flash_index % PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX];
        } else {
            recorded_1 = pedal_pico_looper_buffer_in_1[pedal_pico_looper_flash_index % PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX];
        }
        mixed_1 = normalized_1 + recorded_1;
        int16 to_record_1;
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_RECORDING_BITS) {
            to_record_1 = mixed_1;
        } else {
            to_record_1 = recorded_1;
        }
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS) {
            pedal_pico_looper_buffer_out_2[pedal_pico_looper_flash_index % PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX] = to_record_1;
        } else {
            pedal_pico_looper_buffer_out_1[pedal_pico_looper_flash_index % PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX] = to_record_1;
        }
        pedal_pico_looper_flash_index++;
        if (pedal_pico_looper_flash_index >= pedal_pico_looper_flash_upto) {
            pedal_pico_looper_buffer_status ^= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS;
            pedal_pico_looper_buffer_status |= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS;
        } else if (pedal_pico_looper_flash_index >= (PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * (pedal_pico_looper_flash_offset_index + 1))) {
            if (! (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS)) {
                pedal_pico_looper_buffer_status ^= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS;
                pedal_pico_looper_buffer_status |= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS;
            }
        }
    } else {
        mixed_1 = normalized_1;
    }
    mixed_1 *= PEDAL_PICO_LOOPER_GAIN;
    pedal_pico_looper->output_1 = util_pedal_pico_cutoff_biased(mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, UTIL_PEDAL_PICO_PWM_OFFSET + UTIL_PEDAL_PICO_PWM_PEAK, UTIL_PEDAL_PICO_PWM_OFFSET - UTIL_PEDAL_PICO_PWM_PEAK);
    pedal_pico_looper->output_1_inverted = util_pedal_pico_cutoff_biased(-mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, UTIL_PEDAL_PICO_PWM_OFFSET + UTIL_PEDAL_PICO_PWM_PEAK, UTIL_PEDAL_PICO_PWM_OFFSET - UTIL_PEDAL_PICO_PWM_PEAK);
}

void pedal_pico_looper_flash_handler() {
    uint32 flash_offset;
    if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_ERASE_BITS) {
        util_pedal_pico_flash_erase(pedal_pico_looper_flash_offset, PEDAL_PICO_LOOPER_FLASH_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
        memset((uchar8*)pedal_pico_looper_buffer_out_1, 0, PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
        memset((uchar8*)pedal_pico_looper_buffer_out_2, 0, PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
        for (uint32 i = 0; i < PEDAL_PICO_LOOPER_FLASH_OFFSET_INDEX_MAX; i++) {
            flash_offset = pedal_pico_looper_flash_offset + ((PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE) * i);
            util_pedal_pico_flash_write(flash_offset, (uchar8*)pedal_pico_looper_buffer_out_1, PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
        }
        flash_offset = pedal_pico_looper_flash_offset;
        memcpy((uchar8*)pedal_pico_looper_buffer_in_1, (uchar8*)(flash_offset + XIP_BASE), PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
        flash_offset += PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE;
        memcpy((uchar8*)pedal_pico_looper_buffer_in_2, (uchar8*)(flash_offset + XIP_BASE), PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
        pedal_pico_looper_buffer_status &= ~(PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS);
        pedal_pico_looper_flash_index = 0;
        pedal_pico_looper_flash_offset_index = 0;
        pedal_pico_looper_buffer_status ^= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_ERASE_BITS;
        __dsb();
    } else if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS) {
        uint32 flash_offset = pedal_pico_looper_flash_offset + ((PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE) * pedal_pico_looper_flash_offset_index);
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS) {
            util_pedal_pico_flash_write(flash_offset, (uchar8*)pedal_pico_looper_buffer_out_1, PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
        } else {
            util_pedal_pico_flash_write(flash_offset, (uchar8*)pedal_pico_looper_buffer_out_2, PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
        }
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS) {
            flash_offset = pedal_pico_looper_flash_offset;
            memcpy((uchar8*)pedal_pico_looper_buffer_in_1, (uchar8*)(flash_offset + XIP_BASE), PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
            flash_offset += PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE;
            memcpy((uchar8*)pedal_pico_looper_buffer_in_2, (uchar8*)(flash_offset + XIP_BASE), PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
            pedal_pico_looper_buffer_status &= ~(PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS);
            pedal_pico_looper_flash_index = 0;
            pedal_pico_looper_flash_offset_index = 0;
            pedal_pico_looper_buffer_status ^= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS;
        } else {
            pedal_pico_looper_flash_offset_index++;
            uint32 offset_index = pedal_pico_looper_flash_offset_index + 1;
            if (offset_index >= pedal_pico_looper_flash_offset_upto) offset_index -= pedal_pico_looper_flash_offset_upto;
            flash_offset = pedal_pico_looper_flash_offset + ((PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE) * offset_index);
            if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS) {
                memcpy((uchar8*)pedal_pico_looper_buffer_in_1, (uchar8*)(flash_offset + XIP_BASE), PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
            } else {
                memcpy((uchar8*)pedal_pico_looper_buffer_in_2, (uchar8*)(flash_offset + XIP_BASE), PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
            }
        }
        pedal_pico_looper_buffer_status ^= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS;
        __dsb();
    }
}

void pedal_pico_looper_free() { // Free Except Object, pedal_pico_looper
    free((void*)pedal_pico_looper_buffer_in_1);
    free((void*)pedal_pico_looper_buffer_in_2);
    free((void*)pedal_pico_looper_buffer_out_1);
    free((void*)pedal_pico_looper_buffer_out_2);
    __dsb();
}
