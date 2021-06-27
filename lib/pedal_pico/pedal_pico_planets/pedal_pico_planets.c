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
    int32_t coefficient = (int32_t)((pedal_pico_planets_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT) + 1) * PEDAL_PICO_PLANETS_COEFFICIENT_MULTIPLIER; // Make 5-bit Value (1-32) and Multiply for 32-bit Signed (Two's Compliment) Fixed Decimal
    pedal_pico_planets_coefficient = coefficient;
    pedal_pico_planets_coefficient_interpolation = coefficient;
    pedal_pico_planets_delay_x = (int16_t*)calloc(PEDAL_PICO_PLANETS_DELAY_TIME_MAX, sizeof(int16_t));
    pedal_pico_planets_delay_y = (int16_t*)calloc(PEDAL_PICO_PLANETS_DELAY_TIME_MAX, sizeof(int16_t));
    uint16_t delay_time = ((pedal_pico_planets_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) + 1) << PEDAL_PICO_PLANETS_DELAY_TIME_SHIFT; // Make 5-bit Value (1-32) and Shift
    pedal_pico_planets_delay_time = delay_time;
    pedal_pico_planets_delay_time_interpolation = delay_time;
    pedal_pico_planets_delay_index = 0;
}

void pedal_pico_planets_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (abs(conversion_2 - pedal_pico_planets_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_planets_conversion_2 = conversion_2;
        pedal_pico_planets_coefficient = (int32_t)((pedal_pico_planets_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT) + 1) * PEDAL_PICO_PLANETS_COEFFICIENT_MULTIPLIER; // Make 5-bit Value (1-32) and Multiply for 32-bit Signed (Two's Compliment) Fixed Decimal
    }
    if (abs(conversion_3 - pedal_pico_planets_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_planets_conversion_3 = conversion_3;
        pedal_pico_planets_delay_time = ((pedal_pico_planets_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) + 1) << PEDAL_PICO_PLANETS_DELAY_TIME_SHIFT; // Make 5-bit Value (1-32) and Shift
    }
    pedal_pico_planets_coefficient_interpolation = _interpolate(pedal_pico_planets_coefficient_interpolation, pedal_pico_planets_coefficient, PEDAL_PICO_PLANETS_COEFFICIENT_INTERPOLATION_ACCUM);
    pedal_pico_planets_delay_time_interpolation = _interpolate(pedal_pico_planets_delay_time_interpolation, pedal_pico_planets_delay_time, PEDAL_PICO_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_1);
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    int16_t delay_x = pedal_pico_planets_delay_x[((pedal_pico_planets_delay_index + PEDAL_PICO_PLANETS_DELAY_TIME_MAX) - pedal_pico_planets_delay_time_interpolation) % PEDAL_PICO_PLANETS_DELAY_TIME_MAX];
    int16_t delay_y = pedal_pico_planets_delay_y[((pedal_pico_planets_delay_index + PEDAL_PICO_PLANETS_DELAY_TIME_MAX) - pedal_pico_planets_delay_time_interpolation) % PEDAL_PICO_PLANETS_DELAY_TIME_MAX];
    /* First Stage: High Pass Filter */
    int32_t high_pass_1 = (int32_t)((-(((int64_t)delay_x << 16) * (int64_t)pedal_pico_planets_coefficient_interpolation) + (((int64_t)normalized_1 << 16) * (int64_t)(0x00010000 - pedal_pico_planets_coefficient_interpolation))) >> 32);
    pedal_pico_planets_delay_x[pedal_pico_planets_delay_index] = (int16_t)high_pass_1;
    /* Second Stage: Low Pass Filter */
    if (util_pedal_pico_sw_mode == 1) high_pass_1 = normalized_1; // Bypass High Pass Filter
    int32_t low_pass_1 = (int32_t)(((((int64_t)delay_y << 16) * (int64_t)pedal_pico_planets_coefficient_interpolation) + (((int64_t)high_pass_1 << 16) * (int64_t)(0x00010000 - pedal_pico_planets_coefficient_interpolation))) >> 32);
    pedal_pico_planets_delay_y[pedal_pico_planets_delay_index] = (int16_t)low_pass_1;
    pedal_pico_planets_delay_index++;
    if (pedal_pico_planets_delay_index >= PEDAL_PICO_PLANETS_DELAY_TIME_MAX) pedal_pico_planets_delay_index -= PEDAL_PICO_PLANETS_DELAY_TIME_MAX;
    int32_t mixed_1;
    if (sw_mode == 1) {
        mixed_1 = low_pass_1;
    } else if (sw_mode == 2) {
        mixed_1 = low_pass_1;
    } else {
        mixed_1 = high_pass_1;
    }
    /* Output */
    pedal_pico_planets->output_1 = mixed_1;
    pedal_pico_planets->output_1_inverted = -mixed_1;
}

void pedal_pico_planets_free() { // Free Except Object, pedal_pico_planets
    free((void*)pedal_pico_planets_delay_x);
    free((void*)pedal_pico_planets_delay_y);
    __dsb();
}
