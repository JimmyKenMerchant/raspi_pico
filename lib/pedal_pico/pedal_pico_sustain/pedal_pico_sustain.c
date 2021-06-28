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

#include "pedal_pico/pedal_pico_sustain.h"

void pedal_pico_sustain_set() {
    if (! pedal_pico_sustain) panic("pedal_pico_sustain is not initialized.");
    pedal_pico_sustain_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_sustain_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_sustain_amplitude = (int32_t)(pedal_pico_sustain_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT) << PEDAL_PICO_SUSTAIN_AMPLITUDE_SHIFT; // Make 5-bit Value (0-31) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    pedal_pico_sustain_noise_gate_threshold = (int8_t)((UTIL_PEDAL_PICO_ADC_RESOLUTION + 1) - (pedal_pico_sustain_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT)) * PEDAL_PICO_SUSTAIN_NOISE_GATE_THRESHOLD_MULTIPLIER; // Make 5-bit Value (32-1) and Multiply
    pedal_pico_sustain_is_on = false;
    pedal_pico_sustain_wave_moving_average_sum = 0;
}

void pedal_pico_sustain_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (abs(conversion_2 - pedal_pico_sustain_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_sustain_conversion_2 = conversion_2;
        pedal_pico_sustain_amplitude = (int32_t)(pedal_pico_sustain_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT) << PEDAL_PICO_SUSTAIN_AMPLITUDE_SHIFT; // Make 5-bit Value (0-31) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    }
    if (abs(conversion_3 - pedal_pico_sustain_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_sustain_conversion_3 = conversion_3;
        pedal_pico_sustain_noise_gate_threshold = (int8_t)((UTIL_PEDAL_PICO_ADC_RESOLUTION + 1) - (pedal_pico_sustain_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT)) * PEDAL_PICO_SUSTAIN_NOISE_GATE_THRESHOLD_MULTIPLIER; // Make 5-bit Value (32-1) and Multiply
    }
    /**
     * pedal_pico_sustain_is_on:
     *
     * Over Positive Threshold: True
     *-----------------------------------------------------------------------------------------------------------
     * Under Positive Threshold: No Change
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis: False
     *-----------------------------------------------------------------------------------------------------------
     * 0: = False
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis: False
     *-----------------------------------------------------------------------------------------------------------
     * Under Negative Threshold: No Change
     *-----------------------------------------------------------------------------------------------------------
     * Over Negative Threshold: True
     */
    if (normalized_1 > pedal_pico_sustain_noise_gate_threshold || normalized_1 < -pedal_pico_sustain_noise_gate_threshold) {
        pedal_pico_sustain_is_on = true;
    } else if (normalized_1 < (pedal_pico_sustain_noise_gate_threshold >> PEDAL_PICO_SUSTAIN_NOISE_GATE_HYSTERESIS_SHIFT) && normalized_1 > -(pedal_pico_sustain_noise_gate_threshold >> PEDAL_PICO_SUSTAIN_NOISE_GATE_HYSTERESIS_SHIFT)) {
        pedal_pico_sustain_is_on = false;
    }
    /* Make Sustain */
    int16_t sustain_wave;
    if (pedal_pico_sustain_is_on) {
        if (sw_mode == 1) {
            sustain_wave = PEDAL_PICO_SUSTAIN_PEAK_FIXED_1;
        } else if (sw_mode == 2) {
            sustain_wave = PEDAL_PICO_SUSTAIN_PEAK_FIXED_2;
        } else {
            sustain_wave = PEDAL_PICO_SUSTAIN_PEAK_FIXED_3;
        }
        if (normalized_1 < 0) sustain_wave *= -1;
    } else {
        sustain_wave = 0;
    }
    /* Low-pass filter */
    int32_t wave_moving_average = pedal_pico_sustain_wave_moving_average_sum / PEDAL_PICO_SUSTAIN_WAVE_MOVING_AVERAGE_NUMBER;
    pedal_pico_sustain_wave_moving_average_sum -= wave_moving_average;
    pedal_pico_sustain_wave_moving_average_sum += sustain_wave;
    int32_t low_pass_1 = pedal_pico_sustain_wave_moving_average_sum / PEDAL_PICO_SUSTAIN_WAVE_MOVING_AVERAGE_NUMBER;
    /* Mix */
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    int32_t pedal_pico_sustain_normalized_1_amplitude = 0x00010000 - pedal_pico_sustain_amplitude;
    normalized_1 = (int32_t)((((int64_t)normalized_1 << 16) * (int64_t)pedal_pico_sustain_normalized_1_amplitude) >> 32);
    low_pass_1 = (int32_t)((((int64_t)low_pass_1 << 16) * (int64_t)pedal_pico_sustain_amplitude) >> 32);
    int32_t mixed_1 = normalized_1 + low_pass_1;
    pedal_pico_sustain->output_1 = mixed_1;
    pedal_pico_sustain->output_1_inverted = -mixed_1;
}

void pedal_pico_sustain_free() { // Free Except Object, pedal_pico_sustain
    __dsb();
}
