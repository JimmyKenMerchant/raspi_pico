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

#include "pedal_pico/pedal_pico_buffer.h"

void pedal_pico_buffer_set() {
    if (! pedal_pico_buffer) panic("pedal_pico_buffer is not initialized.");
    pedal_pico_buffer_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_buffer_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_buffer_delay_array = (int16_t*)calloc(PEDAL_PICO_BUFFER_DELAY_TIME_MAX, sizeof(int16_t));
    pedal_pico_buffer_delay_amplitude = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_1;
    pedal_pico_buffer_delay_amplitude_interpolation = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_1;
    pedal_pico_buffer_delay_amplitude_interpolation_accum = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_1;
    uint16_t delay_time = (pedal_pico_buffer_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT) << PEDAL_PICO_BUFFER_DELAY_TIME_SHIFT; // Make 5-bit Value (0-31) and Shift
    pedal_pico_buffer_delay_time = delay_time;
    pedal_pico_buffer_delay_time_interpolation = delay_time;
    pedal_pico_buffer_delay_index = 0;
    pedal_pico_buffer_noise_gate_threshold = (uint16_t)((pedal_pico_buffer_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) * PEDAL_PICO_BUFFER_NOISE_GATE_THRESHOLD_MULTIPLIER); // Make 5-bit Value (0-31) and Multiply
    pedal_pico_buffer_noise_gate_count = 0;
}

void pedal_pico_buffer_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (abs(conversion_2 - pedal_pico_buffer_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_buffer_conversion_2 = conversion_2;
        pedal_pico_buffer_delay_time = (pedal_pico_buffer_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT) << PEDAL_PICO_BUFFER_DELAY_TIME_SHIFT; // Make 5-bit Value (0-31) and Shift
    }
    if (abs(conversion_3 - pedal_pico_buffer_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_buffer_conversion_3 = conversion_3;
        pedal_pico_buffer_noise_gate_threshold = (uint16_t)((pedal_pico_buffer_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) * PEDAL_PICO_BUFFER_NOISE_GATE_THRESHOLD_MULTIPLIER); // Make 5-bit Value (0-31) and Multiply
    }
    pedal_pico_buffer_delay_time_interpolation = _interpolate(pedal_pico_buffer_delay_time_interpolation, pedal_pico_buffer_delay_time, PEDAL_PICO_BUFFER_DELAY_TIME_INTERPOLATION_ACCUM);
    /* Noise Gate */
    pedal_pico_buffer_noise_gate_count = util_pedal_pico_threshold_gate_count(pedal_pico_buffer_noise_gate_count, normalized_1, pedal_pico_buffer_noise_gate_threshold, PEDAL_PICO_BUFFER_NOISE_GATE_HYSTERESIS_SHIFT);
    if (pedal_pico_buffer_noise_gate_count >= PEDAL_PICO_BUFFER_NOISE_GATE_COUNT_MAX) {
        pedal_pico_buffer_noise_gate_count = 0;
        if (sw_mode == 1) {
            pedal_pico_buffer_delay_amplitude = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_1;
            pedal_pico_buffer_delay_amplitude_interpolation_accum = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_1;
        } else if (sw_mode == 2) {
            pedal_pico_buffer_delay_amplitude = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_3;
            pedal_pico_buffer_delay_amplitude_interpolation_accum = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_3;
        } else {
            pedal_pico_buffer_delay_amplitude = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_2;
            pedal_pico_buffer_delay_amplitude_interpolation_accum = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_2;
        }
    } else if (pedal_pico_buffer_noise_gate_count != 0) {
        pedal_pico_buffer_delay_amplitude = 0x00000000;
    }
    pedal_pico_buffer_delay_amplitude_interpolation = _interpolate(pedal_pico_buffer_delay_amplitude_interpolation, pedal_pico_buffer_delay_amplitude, pedal_pico_buffer_delay_amplitude_interpolation_accum);
    if (pedal_pico_buffer_noise_gate_count == 0) {
        normalized_1 = 0;
    }
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    /* Make Sustain */
    int32_t delay_1 = (int32_t)pedal_pico_buffer_delay_array[((pedal_pico_buffer_delay_index + PEDAL_PICO_BUFFER_DELAY_TIME_MAX) - pedal_pico_buffer_delay_time_interpolation) % PEDAL_PICO_BUFFER_DELAY_TIME_MAX];
    if (pedal_pico_buffer_delay_time_interpolation == 0) delay_1 = 0; // No Reverb, Otherwise Latest
    int32_t pedal_pico_buffer_normalized_1_amplitude = 0x00010000 - pedal_pico_buffer_delay_amplitude_interpolation;
    normalized_1 = (int32_t)((((int64_t)normalized_1 << 16) * (int64_t)pedal_pico_buffer_normalized_1_amplitude) >> 32);
    delay_1 = (int32_t)((((int64_t)delay_1 << 16) * (int64_t)pedal_pico_buffer_delay_amplitude_interpolation) >> 32);
    int32_t mixed_1 = normalized_1 + delay_1;
    pedal_pico_buffer_delay_array[pedal_pico_buffer_delay_index] = (int16_t)mixed_1;
    pedal_pico_buffer_delay_index++;
    if (pedal_pico_buffer_delay_index >= PEDAL_PICO_BUFFER_DELAY_TIME_MAX) pedal_pico_buffer_delay_index -= PEDAL_PICO_BUFFER_DELAY_TIME_MAX;
    pedal_pico_buffer->output_1 = mixed_1;
    pedal_pico_buffer->output_1_inverted = -mixed_1;
}

void pedal_pico_buffer_free() { // Free Except Object, pedal_pico_buffer
    free((void*)pedal_pico_buffer_delay_array);
    __dsb();
}
