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

#include "pedal_pico/pedal_pico_tremolo.h"

void pedal_pico_tremolo_set() {
    if (! pedal_pico_tremolo) panic("pedal_pico_tremolo is not initialized.");
    pedal_pico_tremolo_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_tremolo_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_tremolo_osc_triangle_1_index = 0;
    pedal_pico_tremolo_osc_speed = pedal_pico_tremolo_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT; // Make 5-bit Value (0-31)
    pedal_pico_tremolo_osc_is_negative = false;
    pedal_pico_tremolo_osc_start_threshold = (uint16_t)((pedal_pico_tremolo_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) * PEDAL_PICO_TREMOLO_OSC_START_THRESHOLD_MULTIPLIER); // Make 5-bit Value (0-31) and Multiply
    pedal_pico_tremolo_osc_start_count = 0;
    pedal_pico_tremolo_osc_is_faded = false;
}

void pedal_pico_tremolo_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (abs(conversion_2 - pedal_pico_tremolo_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_tremolo_conversion_2 = conversion_2;
        pedal_pico_tremolo_osc_speed = pedal_pico_tremolo_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT; // Make 5-bit Value (0-31)
    }
    if (abs(conversion_3 - pedal_pico_tremolo_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_tremolo_conversion_3 = conversion_3;
        pedal_pico_tremolo_osc_start_threshold = (uint16_t)((pedal_pico_tremolo_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) * PEDAL_PICO_TREMOLO_OSC_START_THRESHOLD_MULTIPLIER); // Make 5-bit Value (0-31) and Multiply
    }
    /* Oscillator Start */
    pedal_pico_tremolo_osc_start_count = util_pedal_pico_threshold_gate_count(pedal_pico_tremolo_osc_start_count, normalized_1, pedal_pico_tremolo_osc_start_threshold, PEDAL_PICO_TREMOLO_OSC_START_HYSTERESIS_SHIFT);
    if (pedal_pico_tremolo_osc_start_count >= PEDAL_PICO_TREMOLO_OSC_START_COUNT_MAX) pedal_pico_tremolo_osc_start_count = 0;
    if (pedal_pico_tremolo_osc_start_count == 0) {
        pedal_pico_tremolo_osc_triangle_1_index = 0;
        pedal_pico_tremolo_osc_is_negative = false;
        pedal_pico_tremolo_osc_is_faded = false;
    }
    /* Get Oscillator */
    int32_t fixed_point_value_triangle_1 = util_pedal_pico_table_triangle_1[abs(pedal_pico_tremolo_osc_triangle_1_index) / PEDAL_PICO_TREMOLO_OSC_TRIANGLE_1_TIME_MULTIPLIER]; // Depending on osc_spped, the index value may have a negative value.
    pedal_pico_tremolo_osc_is_negative ? (pedal_pico_tremolo_osc_triangle_1_index -= pedal_pico_tremolo_osc_speed) : (pedal_pico_tremolo_osc_triangle_1_index += pedal_pico_tremolo_osc_speed);
    if (pedal_pico_tremolo_osc_triangle_1_index >= UTIL_PEDAL_PICO_OSC_TRIANGLE_1_TIME_MAX * PEDAL_PICO_TREMOLO_OSC_TRIANGLE_1_TIME_MULTIPLIER) {
        pedal_pico_tremolo_osc_triangle_1_index = (UTIL_PEDAL_PICO_OSC_TRIANGLE_1_TIME_MAX * PEDAL_PICO_TREMOLO_OSC_TRIANGLE_1_TIME_MULTIPLIER) - 1;
        pedal_pico_tremolo_osc_is_negative = true;
        if (! pedal_pico_tremolo_osc_is_faded) pedal_pico_tremolo_osc_is_faded = true;
    } else if (pedal_pico_tremolo_osc_triangle_1_index <= 0) {
        pedal_pico_tremolo_osc_triangle_1_index = 0;
        pedal_pico_tremolo_osc_is_negative = false;
    }
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    if (sw_mode == 1) { // Inverted Shallow
        fixed_point_value_triangle_1 >>= 1;
    } else if (sw_mode == 2) { // Fade In
        if (pedal_pico_tremolo_osc_is_faded) {
            fixed_point_value_triangle_1 = 0x00010000;
        }
    }
    if (sw_mode == 1) {
        normalized_1 = (int32_t)((((int64_t)normalized_1 << 16) * (int64_t)(0x00010000 - fixed_point_value_triangle_1)) >> 32);
    } else {
        normalized_1 = (int32_t)((((int64_t)normalized_1 << 16) * (int64_t)fixed_point_value_triangle_1) >> 32);
    }
    pedal_pico_tremolo->output_1 = normalized_1;
    pedal_pico_tremolo->output_1_inverted = -normalized_1;
}

void pedal_pico_tremolo_free() { // Free Except Object, pedal_pico_tremolo
    __dsb();
}
