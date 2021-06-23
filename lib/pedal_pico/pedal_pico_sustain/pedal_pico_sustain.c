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
    pedal_pico_sustain_delay_array = (int16*)calloc(PEDAL_PICO_SUSTAIN_DELAY_TIME_MAX, sizeof(int16));
    pedal_pico_sustain_delay_amplitude = (int32)(pedal_pico_sustain_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT) << PEDAL_PICO_SUSTAIN_DELAY_AMPLITUDE_SHIFT; // Make 5-bit Value (0-31) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    pedal_pico_sustain_delay_time = PEDAL_PICO_SUSTAIN_DELAY_TIME_FIXED_1;
    pedal_pico_sustain_delay_index = 0;
    pedal_pico_sustain_noise_gate_threshold = (char8)((UTIL_PEDAL_PICO_ADC_RESOLUTION + 1) - (pedal_pico_sustain_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT)) * PEDAL_PICO_SUSTAIN_NOISE_GATE_THRESHOLD_MULTIPLIER; // Make 5-bit Value (32-1) and Multiply
    pedal_pico_sustain_is_on = false;
}

void pedal_pico_sustain_process(int32 normalized_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    if (abs(conversion_2 - pedal_pico_sustain_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_sustain_conversion_2 = conversion_2;
        pedal_pico_sustain_delay_amplitude = (int32)(pedal_pico_sustain_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT) << PEDAL_PICO_SUSTAIN_DELAY_AMPLITUDE_SHIFT; // Make 5-bit Value (0-31) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    }
    if (abs(conversion_3 - pedal_pico_sustain_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_sustain_conversion_3 = conversion_3;
        pedal_pico_sustain_noise_gate_threshold = (char8)((UTIL_PEDAL_PICO_ADC_RESOLUTION + 1) - (pedal_pico_sustain_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT)) * PEDAL_PICO_SUSTAIN_NOISE_GATE_THRESHOLD_MULTIPLIER; // Make 5-bit Value (32-1) and Multiply
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
    int16 sustain_wave;
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
    /* Low-pass Filter */
    pedal_pico_sustain_delay_array[pedal_pico_sustain_delay_index] = sustain_wave;
    int32 low_pass_1 = 0;
    for (uint16 i = 0; i < pedal_pico_sustain_delay_time; i++) {
        low_pass_1 += (int32)pedal_pico_sustain_delay_array[((pedal_pico_sustain_delay_index + PEDAL_PICO_SUSTAIN_DELAY_TIME_MAX) - i) % PEDAL_PICO_SUSTAIN_DELAY_TIME_MAX];
    }
    low_pass_1 = low_pass_1 / pedal_pico_sustain_delay_time;
    pedal_pico_sustain_delay_index++;
    if (pedal_pico_sustain_delay_index >= PEDAL_PICO_SUSTAIN_DELAY_TIME_MAX) pedal_pico_sustain_delay_index -= PEDAL_PICO_SUSTAIN_DELAY_TIME_MAX;
    /* Mix */
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    int32 pedal_pico_sustain_normalized_1_amplitude = 0x00010000 - pedal_pico_sustain_delay_amplitude;
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_sustain_normalized_1_amplitude) >> 32);
    low_pass_1 = (int32)(int64)((((int64)low_pass_1 << 16) * (int64)pedal_pico_sustain_delay_amplitude) >> 32);
    int32 mixed_1 = normalized_1 + low_pass_1;
    pedal_pico_sustain->output_1 = mixed_1;
    pedal_pico_sustain->output_1_inverted = -mixed_1;
}

void pedal_pico_sustain_free() { // Free Except Object, pedal_pico_sustain
    free((void*)pedal_pico_sustain_delay_array);
    __dsb();
}
