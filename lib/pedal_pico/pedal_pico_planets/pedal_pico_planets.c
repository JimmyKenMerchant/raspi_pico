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

#include "pedal_pico/pedal_pico_planets.h"

void pedal_pico_planets_set() {
    if (! pedal_pico_planets) panic("pedal_pico_planets is not initialized.");
    pedal_pico_planets_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_planets_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_planets_delay_y = (int16_t*)calloc(PEDAL_PICO_PLANETS_DELAY_TIME_MAX, sizeof(int16_t));
    pedal_pico_planets_delay_y_index = 0;
    uint16_t delay_time_high_pass = ((UTIL_PEDAL_PICO_ADC_FINE_RESOLUTION + 1) - (pedal_pico_planets_conversion_2 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT)) << PEDAL_PICO_PLANETS_DELAY_TIME_SHIFT; // Make 5-bit Value (32-1) and Shift
    pedal_pico_planets_delay_time_high_pass = delay_time_high_pass;
    pedal_pico_planets_delay_time_high_pass_interpolation = delay_time_high_pass;
    uint16_t delay_time_low_pass = ((UTIL_PEDAL_PICO_ADC_FINE_RESOLUTION + 1) - (pedal_pico_planets_conversion_3 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT)) << PEDAL_PICO_PLANETS_DELAY_TIME_SHIFT; // Make 5-bit Value (32-1) and Shift
    pedal_pico_planets_delay_time_low_pass = delay_time_low_pass;
    pedal_pico_planets_delay_time_low_pass_interpolation = delay_time_low_pass;
    pedal_pico_planets_moving_average_sum_low_pass = 0;
}

void pedal_pico_planets_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (abs(conversion_2 - pedal_pico_planets_conversion_2) >= UTIL_PEDAL_PICO_ADC_FINE_THRESHOLD) {
        pedal_pico_planets_conversion_2 = conversion_2;
        pedal_pico_planets_delay_time_high_pass = ((UTIL_PEDAL_PICO_ADC_FINE_RESOLUTION + 1) - (pedal_pico_planets_conversion_2 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT)) << PEDAL_PICO_PLANETS_DELAY_TIME_SHIFT; // Make 5-bit Value (32-1) and Shift
    }
    if (abs(conversion_3 - pedal_pico_planets_conversion_3) >= UTIL_PEDAL_PICO_ADC_FINE_THRESHOLD) {
        pedal_pico_planets_conversion_3 = conversion_3;
        pedal_pico_planets_delay_time_low_pass = ((UTIL_PEDAL_PICO_ADC_FINE_RESOLUTION + 1) - (pedal_pico_planets_conversion_3 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT)) << PEDAL_PICO_PLANETS_DELAY_TIME_SHIFT; // Make 5-bit Value (32-1) and Shift
    }
    pedal_pico_planets_delay_time_high_pass_interpolation = _interpolate(pedal_pico_planets_delay_time_high_pass_interpolation, pedal_pico_planets_delay_time_high_pass, PEDAL_PICO_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_1);
    pedal_pico_planets_delay_time_low_pass_interpolation = _interpolate(pedal_pico_planets_delay_time_low_pass_interpolation, pedal_pico_planets_delay_time_low_pass, PEDAL_PICO_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_1);
    if (sw_mode == 1) {
        pedal_pico_planets_delay_time_low_pass_interpolation = pedal_pico_planets_delay_time_high_pass_interpolation;
    }
    /* High-pass Filter */
    int16_t delay_y = pedal_pico_planets_delay_y[((pedal_pico_planets_delay_y_index + PEDAL_PICO_PLANETS_DELAY_TIME_MAX) - pedal_pico_planets_delay_time_high_pass_interpolation) % PEDAL_PICO_PLANETS_DELAY_TIME_MAX];
    /* First Stage: High Pass Filter */
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    int32_t high_pass_1 = (int32_t)((-(((int64_t)delay_y << 16) * (int64_t)PEDAL_PICO_PLANETS_COEFFICIENT_FIXED_1) + (((int64_t)normalized_1 << 16) * (int64_t)(0x00010000 - PEDAL_PICO_PLANETS_COEFFICIENT_FIXED_1))) >> 32);
    pedal_pico_planets_delay_y[pedal_pico_planets_delay_y_index] = (int16_t)high_pass_1;
    pedal_pico_planets_delay_y_index++;
    if (pedal_pico_planets_delay_y_index >= PEDAL_PICO_PLANETS_DELAY_TIME_MAX) pedal_pico_planets_delay_y_index -= PEDAL_PICO_PLANETS_DELAY_TIME_MAX;
    /* Second Stage: Low Pass Filter */
    int32_t low_pass_1;
    pedal_pico_planets_moving_average_sum_low_pass -= (pedal_pico_planets_moving_average_sum_low_pass / (int32_t)pedal_pico_planets_delay_time_low_pass_interpolation);
    pedal_pico_planets_moving_average_sum_low_pass += high_pass_1;
    low_pass_1 = pedal_pico_planets_moving_average_sum_low_pass / (int32_t)pedal_pico_planets_delay_time_low_pass_interpolation;
    int32_t mixed_1;
    if (sw_mode == 1) {
        mixed_1 = low_pass_1;
    } else if (sw_mode == 2) {
        mixed_1 = high_pass_1;
    } else {
        mixed_1 = low_pass_1;
    }
    /* Output */
    pedal_pico_planets->output_1 = mixed_1;
    pedal_pico_planets->output_1_inverted = -mixed_1;
}

void pedal_pico_planets_free() { // Free Except Object, pedal_pico_planets
    free((void*)pedal_pico_planets_delay_y);
    __dsb();
}
