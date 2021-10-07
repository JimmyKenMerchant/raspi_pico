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

#include "pedal_pico/pedal_pico_tape.h"

void pedal_pico_tape_set() {
    if (! pedal_pico_tape) panic("pedal_pico_tape is not initialized.");
    pedal_pico_tape_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_tape_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_tape_delay_array = (int16_t*)calloc(PEDAL_PICO_TAPE_DELAY_TIME_MAX, sizeof(int16_t));
    pedal_pico_tape_delay_amplitude = PEDAL_PICO_TAPE_DELAY_AMPLITUDE_PEAK_FIXED_1;
    pedal_pico_tape_delay_time = PEDAL_PICO_TAPE_DELAY_TIME_FIXED_1;
    pedal_pico_tape_delay_time_swing = (pedal_pico_tape_conversion_2 >> UTIL_PEDAL_PICO_ADC_COARSE_SHIFT) << PEDAL_PICO_TAPE_DELAY_TIME_SWING_SHIFT; // Make 5-bit Value (0-31) and Shift
    pedal_pico_tape_delay_index = 0;
    pedal_pico_tape_osc_speed = pedal_pico_tape_conversion_3 >> UTIL_PEDAL_PICO_ADC_COARSE_SHIFT; // Make 5-bit Value (0-31)
    pedal_pico_tape_osc_sine_1_index = 0;
    pedal_pico_tape_osc_is_negative = false;
}

void pedal_pico_tape_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (abs(conversion_2 - pedal_pico_tape_conversion_2) > UTIL_PEDAL_PICO_ADC_COARSE_THRESHOLD) {
        pedal_pico_tape_conversion_2 = conversion_2;
        pedal_pico_tape_delay_time_swing = (pedal_pico_tape_conversion_2 >> UTIL_PEDAL_PICO_ADC_COARSE_SHIFT) << PEDAL_PICO_TAPE_DELAY_TIME_SWING_SHIFT; // Make 5-bit Value (0-31) and Shift
    }
    if (abs(conversion_3 - pedal_pico_tape_conversion_3) > UTIL_PEDAL_PICO_ADC_COARSE_THRESHOLD) {
        pedal_pico_tape_conversion_3 = conversion_3;
        pedal_pico_tape_osc_speed = pedal_pico_tape_conversion_3 >> UTIL_PEDAL_PICO_ADC_COARSE_SHIFT; // Make 5-bit Value (0-31)
    }
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    /* Get Oscillator */
    int32_t fixed_point_value_sine_1 = util_pedal_pico_table_sine_1[pedal_pico_tape_osc_sine_1_index / PEDAL_PICO_TAPE_OSC_SINE_1_TIME_MULTIPLIER];
    pedal_pico_tape_osc_sine_1_index += pedal_pico_tape_osc_speed;
    if (pedal_pico_tape_osc_sine_1_index >= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_TAPE_OSC_SINE_1_TIME_MULTIPLIER) {
        pedal_pico_tape_osc_sine_1_index -= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_TAPE_OSC_SINE_1_TIME_MULTIPLIER;
        pedal_pico_tape_osc_is_negative ^= true;
    }
    if (pedal_pico_tape_osc_is_negative) fixed_point_value_sine_1 *= -1;
    if (sw_mode == 1) {
        fixed_point_value_sine_1 >>= 2;
    } else {
        fixed_point_value_sine_1 >>= 1;
    }
    int16_t time_swing = (int16_t)(int64_t)((((int64_t)pedal_pico_tape_delay_time_swing << 16) * (int64_t)fixed_point_value_sine_1) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    int32_t delay_1 = (int32_t)pedal_pico_tape_delay_array[((pedal_pico_tape_delay_index + PEDAL_PICO_TAPE_DELAY_TIME_MAX) - ((int16_t)pedal_pico_tape_delay_time + time_swing)) % PEDAL_PICO_TAPE_DELAY_TIME_MAX];
    if (pedal_pico_tape_delay_time + time_swing == 0) delay_1 = 0; // No Delay, Otherwise Latest
    int32_t pedal_pico_tape_normalized_1_amplitude = 0x00010000 - pedal_pico_tape_delay_amplitude;
    normalized_1 = (int32_t)((((int64_t)normalized_1 << 16) * (int64_t)pedal_pico_tape_normalized_1_amplitude) >> 32);
    delay_1 = (int32_t)((((int64_t)delay_1 << 16) * (int64_t)pedal_pico_tape_delay_amplitude) >> 32);
    int32_t mixed_1 = normalized_1 + delay_1;
    pedal_pico_tape_delay_array[pedal_pico_tape_delay_index] = (int16_t)mixed_1;
    pedal_pico_tape_delay_index++;
    if (pedal_pico_tape_delay_index >= PEDAL_PICO_TAPE_DELAY_TIME_MAX) pedal_pico_tape_delay_index -= PEDAL_PICO_TAPE_DELAY_TIME_MAX;
    /* Output */
    pedal_pico_tape->output_1 = mixed_1;
    pedal_pico_tape->output_1_inverted = -mixed_1;
}

void pedal_pico_tape_free() { // Free Except Object, pedal_pico_tape
    free((void*)pedal_pico_tape_delay_array);
    __dsb();
}
