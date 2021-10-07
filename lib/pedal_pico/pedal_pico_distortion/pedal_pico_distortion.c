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

#include "pedal_pico/pedal_pico_distortion.h"

void pedal_pico_distortion_set() {
    if (! pedal_pico_distortion) panic("pedal_pico_distortion is not initialized.");
    pedal_pico_distortion_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_distortion_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
}

void pedal_pico_distortion_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (abs(conversion_2 - pedal_pico_distortion_conversion_2) > UTIL_PEDAL_PICO_ADC_COARSE_THRESHOLD) {
        pedal_pico_distortion_conversion_2 = conversion_2;
    }
    if (abs(conversion_3 - pedal_pico_distortion_conversion_3) > UTIL_PEDAL_PICO_ADC_COARSE_THRESHOLD) {
        pedal_pico_distortion_conversion_3 = conversion_3;
    }
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_DISTORTION_CUTOFF_FIXED_1);
    if (sw_mode == 1) {
        if (normalized_1 > 0) {
            normalized_1 = (int32_t)((((int64_t)normalized_1 << 16) * (int64_t)util_pedal_pico_table_log_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, UTIL_PEDAL_PICO_PWM_PEAK))]) >> 32);
        } else {
            normalized_1 = (int32_t)((((int64_t)normalized_1 << 16) * (int64_t)util_pedal_pico_table_power_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, UTIL_PEDAL_PICO_PWM_PEAK))]) >> 32);
        }
    } else if (sw_mode == 2) {
        if (normalized_1 > 0) {
            normalized_1 = (int32_t)((((int64_t)normalized_1 << 16) * (int64_t)util_pedal_pico_table_log_2[abs(util_pedal_pico_cutoff_normalized(normalized_1, UTIL_PEDAL_PICO_PWM_PEAK))]) >> 32);
        } else {
            normalized_1 = (int32_t)((((int64_t)normalized_1 << 16) * (int64_t)util_pedal_pico_table_log_2[abs(util_pedal_pico_cutoff_normalized(normalized_1, UTIL_PEDAL_PICO_PWM_PEAK))]) >> 32);
        }
    } else {
        if (normalized_1 > 0) {
            normalized_1 = (int32_t)((((int64_t)normalized_1 << 16) * (int64_t)util_pedal_pico_table_log_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, UTIL_PEDAL_PICO_PWM_PEAK))]) >> 32);
        } else {
            normalized_1 = (int32_t)((((int64_t)normalized_1 << 16) * (int64_t)util_pedal_pico_table_log_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, UTIL_PEDAL_PICO_PWM_PEAK))]) >> 32);
        }
    }
    /* Output */
    pedal_pico_distortion->output_1 = normalized_1;
    pedal_pico_distortion->output_1_inverted = -normalized_1;
}

void pedal_pico_distortion_free() { // Free Except Object, pedal_pico_distortion
    __dsb();
}
