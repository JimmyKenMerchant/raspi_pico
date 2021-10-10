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

void pedal_pico_looper_set(uint8_t indicator_led_gpio) {
    if (! pedal_pico_looper) panic("pedal_pico_looper is not initialized.");
    pedal_pico_looper_indicator_led = indicator_led_gpio;
    pedal_pico_looper_indicator_led_bit = 0b1 << indicator_led_gpio;
    pedal_pico_looper_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_looper_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_looper_loss = (UTIL_PEDAL_PICO_ADC_FINE_RESOLUTION + 1) - (pedal_pico_looper_conversion_2 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT); // Make 5-bit Value (1-32)
    //uint8_t* binary_end = (uint8_t*)&__flash_binary_end;
    pedal_pico_looper_flash = (uint8_t*)(&__pedal_pico_looper_flash + FLASH_SECTOR_SIZE); // Offset for Reserve
    if ((uint32_t)pedal_pico_looper_flash % FLASH_SECTOR_SIZE) pedal_pico_looper_flash = (uint8_t*)(((uint32_t)pedal_pico_looper_flash + FLASH_SECTOR_SIZE) & ~(0xFFFFFFFF & (FLASH_SECTOR_SIZE - 1))); // 4096-byte (1024 Words) Aligned Sector (256-byte Aligned Page)
    pedal_pico_looper_flash_index = 0;
    pedal_pico_looper_flash_upto = PEDAL_PICO_LOOPER_FLASH_INDEX_MAX;
    pedal_pico_looper_flash_offset = (uint32_t)pedal_pico_looper_flash - XIP_BASE;
    pedal_pico_looper_flash_reserve_offset = (uint32_t)pedal_pico_looper_flash_reserve - XIP_BASE;
    pedal_pico_looper_flash_offset_index = 0;
    pedal_pico_looper_flash_offset_upto = PEDAL_PICO_LOOPER_FLASH_OFFSET_INDEX_MAX;
    pedal_pico_looper_buffer_in_1 = (uint16_t*)calloc(PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX, sizeof(uint16_t));
    pedal_pico_looper_buffer_in_2 = (uint16_t*)calloc(PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX, sizeof(uint16_t));
    pedal_pico_looper_buffer_out_1 = (uint16_t*)calloc(PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX, sizeof(uint16_t));
    pedal_pico_looper_buffer_out_2 = (uint16_t*)calloc(PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX, sizeof(uint16_t));
    pedal_pico_looper_sw_mode = 0;
    pedal_pico_looper_sw_count = 0;
    pedal_pico_looper_led_toggle_count_on_erase = 0;
    //printf("@pedal_pico_looper_set 1 - pedal_pico_looper_flash_reserve %0x\n", pedal_pico_looper_flash_reserve);
    //printf("@pedal_pico_looper_set 2 - pedal_pico_looper_flash_reserve[FLASH_SECTOR_SIZE - 1] %0x\n", pedal_pico_looper_flash_reserve[FLASH_SECTOR_SIZE - 1]);
    //printf("@pedal_pico_looper_set 3 - pedal_pico_looper_flash %0x\n", pedal_pico_looper_flash);
    if (pedal_pico_looper_flash_reserve[FLASH_SECTOR_SIZE - 1] == 0x88) { // Check the last to know whether flash memory is initialized or not.
        pedal_pico_looper_buffer_status = PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_RESET_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_ERASE_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS; // First Set
    } else {
        pedal_pico_looper_buffer_status = PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_RESET_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS; // First Set
    }
    gpio_init(pedal_pico_looper_indicator_led);
    gpio_set_dir(pedal_pico_looper_indicator_led, GPIO_OUT);
    gpio_put(pedal_pico_looper_indicator_led, 0);
}

void pedal_pico_looper_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (abs(conversion_2 - pedal_pico_looper_conversion_2) >= UTIL_PEDAL_PICO_ADC_FINE_THRESHOLD) {
        pedal_pico_looper_conversion_2 = conversion_2;
        pedal_pico_looper_loss = (UTIL_PEDAL_PICO_ADC_FINE_RESOLUTION + 1) - (pedal_pico_looper_conversion_2 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT); // Make 5-bit Value (1-32)
    }
    if (abs(conversion_3 - pedal_pico_looper_conversion_3) >= UTIL_PEDAL_PICO_ADC_FINE_THRESHOLD) {
        pedal_pico_looper_conversion_3 = conversion_3;
    }
    // LED Indication on Erasing
    if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_ERASE_BITS) {
        pedal_pico_looper_led_toggle_count_on_erase++;
        if (pedal_pico_looper_led_toggle_count_on_erase >= PEDAL_PICO_LOOPER_INDICATOR_LED_TOGGLE_COUNT_ON_ERASE_MAX) {
            pedal_pico_looper_led_toggle_count_on_erase -= PEDAL_PICO_LOOPER_INDICATOR_LED_TOGGLE_COUNT_ON_ERASE_MAX;
            gpio_xor_mask(pedal_pico_looper_indicator_led_bit);
        }
    } else if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS) {
        gpio_put(pedal_pico_looper_indicator_led, 0);
    }
    if (sw_mode == 1) {
        if (! (pedal_pico_looper_buffer_status & (PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_RECORDING_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_RESET_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_ORDER_REWIND_BITS))) {
            if (pedal_pico_looper_sw_mode != sw_mode) {
                pedal_pico_looper_sw_mode = sw_mode;
                pedal_pico_looper_buffer_status = PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_RESET_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS; // Exclusively, Also Remove Status of Double Buffer, etc.
            }
            pedal_pico_looper_sw_count++;
            if (pedal_pico_looper_sw_count >= PEDAL_PICO_LOOPER_FOOT_SW_RESET_THRESHOLD) {
                pedal_pico_looper_sw_count -= PEDAL_PICO_LOOPER_FOOT_SW_RESET_THRESHOLD;
                pedal_pico_looper_buffer_status = PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_RESET_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_ERASE_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS; // Exclusively, Also Remove Status of Double Buffer, etc.
                gpio_put(pedal_pico_looper_indicator_led, 0);
            }
        }
    } else if (sw_mode == 2) {
        if (! (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_RESET_BITS)) {
            if (pedal_pico_looper_sw_mode != sw_mode) {
                pedal_pico_looper_sw_mode = sw_mode;
                if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS) { // After Reset
                    pedal_pico_looper_buffer_status &= ~(PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS);
                } else {
                    if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_RECORDING_BITS) { // Amid Recording
                        pedal_pico_looper_flash_upto = pedal_pico_looper_flash_index;
                        pedal_pico_looper_flash_offset_upto = (pedal_pico_looper_flash_upto % PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX) ? ((pedal_pico_looper_flash_upto / PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX) + 1) : (pedal_pico_looper_flash_upto / PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX);
                        pedal_pico_looper_buffer_status |= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_ORDER_REWIND_BITS;
                        gpio_put(pedal_pico_looper_indicator_led, 0);
                    } else { // At Not Recording, but Playing
                        pedal_pico_looper_buffer_status |= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_RESET_BITS;
                        gpio_put(pedal_pico_looper_indicator_led, 1);
                    }
                    pedal_pico_looper_buffer_status ^= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_RECORDING_BITS; // Toggle Recording Status
                }
            }
        }
    } else {
        if (pedal_pico_looper_sw_mode != sw_mode) {
            pedal_pico_looper_sw_mode = sw_mode;
        }
        pedal_pico_looper_sw_count = 0;
    }
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = normalized_1 / pedal_pico_looper_loss;
    int32_t recorded_1;
    int32_t mixed_1;
    if (! (pedal_pico_looper_buffer_status & (PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_RESET_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_ORDER_REWIND_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_PENDING_BITS))) {
       /**
        * Flash memory becomes all-set after erasing. This means -1 in a signed integer.
        * On casting 16-bit signed to 32-bit signed, the all-set value is expanded to save the minus value.
        * In this program, to get quick erasing, writing to all-zeros is omitted.
        */
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS) {
            recorded_1 = (int32_t)pedal_pico_looper_buffer_in_2[pedal_pico_looper_flash_index % PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX];
        } else {
            recorded_1 = (int32_t)pedal_pico_looper_buffer_in_1[pedal_pico_looper_flash_index % PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX];
        }
        mixed_1 = normalized_1 + recorded_1;
        int32_t to_record_1;
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_RECORDING_BITS) {
            to_record_1 = mixed_1;
        } else {
            to_record_1 = recorded_1;
        }
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS) {
            pedal_pico_looper_buffer_out_2[pedal_pico_looper_flash_index % PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX] = (int16_t)to_record_1;
        } else {
            pedal_pico_looper_buffer_out_1[pedal_pico_looper_flash_index % PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX] = (int16_t)to_record_1;
        }
        pedal_pico_looper_flash_index++;
        if (pedal_pico_looper_flash_index >= pedal_pico_looper_flash_upto) {
            pedal_pico_looper_buffer_status |= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_ORDER_REWIND_BITS; // Clear Except Order to Rewind
        } else if (pedal_pico_looper_flash_index >= (PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * (pedal_pico_looper_flash_offset_index + 1))) {
            if (! (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS)) {
                pedal_pico_looper_buffer_status ^= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS;
                pedal_pico_looper_buffer_status |= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS;
            }
        }
    } else {
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_ORDER_REWIND_BITS) {
            if (! (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS)) {
                pedal_pico_looper_buffer_status ^= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS;
                pedal_pico_looper_buffer_status |= PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS;
                pedal_pico_looper_buffer_status &= ~(PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_ORDER_REWIND_BITS);
            }
        }
        mixed_1 = normalized_1;
    }
    pedal_pico_looper->output_1 = mixed_1;
    pedal_pico_looper->output_1_inverted = -mixed_1;
}

void pedal_pico_looper_flash_handler() {
    uint32_t flash_offset;
    if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_RESET_BITS) {
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_ERASE_BITS) {
            util_pedal_pico_flash_erase(pedal_pico_looper_flash_offset, PEDAL_PICO_LOOPER_FLASH_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE); // All-set
            util_pedal_pico_flash_erase(pedal_pico_looper_flash_reserve_offset, FLASH_SECTOR_SIZE); // All-set
        }
        flash_offset = pedal_pico_looper_flash_offset;
        memcpy((uint8_t*)pedal_pico_looper_buffer_in_1, (uint8_t*)(flash_offset + XIP_BASE), PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
        flash_offset += PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE;
        memcpy((uint8_t*)pedal_pico_looper_buffer_in_2, (uint8_t*)(flash_offset + XIP_BASE), PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
        pedal_pico_looper_flash_index = 0;
        pedal_pico_looper_flash_upto = PEDAL_PICO_LOOPER_FLASH_INDEX_MAX;
        pedal_pico_looper_flash_offset_index = 0;
        pedal_pico_looper_flash_offset_upto = PEDAL_PICO_LOOPER_FLASH_OFFSET_INDEX_MAX;
        pedal_pico_looper_buffer_status &= ~(PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_RESET_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_ERASE_BITS);
        __dsb();
    } else if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS) {
        uint32_t flash_offset = pedal_pico_looper_flash_offset + ((PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE) * pedal_pico_looper_flash_offset_index);
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS) {
            util_pedal_pico_flash_write(flash_offset, (uint8_t*)pedal_pico_looper_buffer_out_1, PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
        } else {
            util_pedal_pico_flash_write(flash_offset, (uint8_t*)pedal_pico_looper_buffer_out_2, PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
        }
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS) {
            flash_offset = pedal_pico_looper_flash_offset;
            memcpy((uint8_t*)pedal_pico_looper_buffer_in_1, (uint8_t*)(flash_offset + XIP_BASE), PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
            flash_offset += PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE;
            memcpy((uint8_t*)pedal_pico_looper_buffer_in_2, (uint8_t*)(flash_offset + XIP_BASE), PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
            pedal_pico_looper_flash_index = 0;
            pedal_pico_looper_flash_offset_index = 0;
        } else {
            pedal_pico_looper_flash_offset_index++;
            uint32_t offset_index = pedal_pico_looper_flash_offset_index + 1;
            if (offset_index >= pedal_pico_looper_flash_offset_upto) offset_index -= pedal_pico_looper_flash_offset_upto;
            flash_offset = pedal_pico_looper_flash_offset + ((PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE) * offset_index);
            if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS) {
                memcpy((uint8_t*)pedal_pico_looper_buffer_in_1, (uint8_t*)(flash_offset + XIP_BASE), PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
            } else {
                memcpy((uint8_t*)pedal_pico_looper_buffer_in_2, (uint8_t*)(flash_offset + XIP_BASE), PEDAL_PICO_LOOPER_BUFFER_INDEX_MAX * PEDAL_PICO_LOOPER_BUFFER_BLOCK_SIZE);
            }
        }
        if (pedal_pico_looper_buffer_status & PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS) {
            pedal_pico_looper_buffer_status &= ~(PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_DOUBLE_BUFFER_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS|PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_REWIND_BITS);
        } else {
            pedal_pico_looper_buffer_status &= ~(PEDAL_PICO_LOOPER_FLASH_BUFFER_STATUS_OUTSTANDING_WRITE_BITS);
        }
        __dsb();
    }
}

void pedal_pico_looper_free() { // Free Except Object, pedal_pico_looper
    free((void*)pedal_pico_looper_buffer_in_1);
    free((void*)pedal_pico_looper_buffer_in_2);
    free((void*)pedal_pico_looper_buffer_out_1);
    free((void*)pedal_pico_looper_buffer_out_2);
    gpio_init(pedal_pico_looper_indicator_led);
    __dsb();
}
