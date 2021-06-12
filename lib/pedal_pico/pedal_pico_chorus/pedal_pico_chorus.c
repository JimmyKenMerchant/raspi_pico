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

#include "pedal_pico/pedal_pico_chorus.h"

void pedal_pico_chorus_set() {
    if (! pedal_pico_chorus) panic("pedal_pico_chorus is not initialized.");
    pedal_pico_chorus_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_chorus_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_chorus_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_chorus_delay_array = (int16*)calloc(PEDAL_PICO_CHORUS_DELAY_TIME_MAX, sizeof(int16));
    pedal_pico_chorus_delay_amplitude = PEDAL_PICO_CHORUS_DELAY_AMPLITUDE_FIXED_1;
    pedal_pico_chorus_delay_time = PEDAL_PICO_CHORUS_DELAY_TIME_FIXED_1;
    pedal_pico_chorus_delay_index = 0;
    pedal_pico_chorus_osc_speed = pedal_pico_chorus_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT; // Make 5-bit Value (0-31)
    pedal_pico_chorus_osc_sine_1_index = 0;
    pedal_pico_chorus_lr_distance_array =  (int16*)calloc(PEDAL_PICO_CHORUS_LR_DISTANCE_TIME_MAX, sizeof(int16));
    uint16 lr_distance_time = (pedal_pico_chorus_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) << PEDAL_PICO_CHORUS_LR_DISTANCE_TIME_SHIFT; // Make 5-bit Value (0-31) and Shift
    pedal_pico_chorus_lr_distance_time = lr_distance_time;
    pedal_pico_chorus_lr_distance_time_interpolation = lr_distance_time;
    pedal_pico_chorus_lr_distance_index = 0;
}

void pedal_pico_chorus_process(int32 normalized_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    if (abs(conversion_2 - pedal_pico_chorus_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_chorus_conversion_2 = conversion_2;
        pedal_pico_chorus_osc_speed = pedal_pico_chorus_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT; // Make 5-bit Value (0-31)
    }
    if (abs(conversion_3 - pedal_pico_chorus_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_chorus_conversion_3 = conversion_3;
        pedal_pico_chorus_lr_distance_time = (pedal_pico_chorus_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) << PEDAL_PICO_CHORUS_LR_DISTANCE_TIME_SHIFT; // Make 5-bit Value (0-31) and Shift
    }
    pedal_pico_chorus_lr_distance_time_interpolation = _interpolate(pedal_pico_chorus_lr_distance_time_interpolation, pedal_pico_chorus_lr_distance_time, PEDAL_PICO_CHORUS_LR_DISTANCE_TIME_INTERPOLATION_ACCUM);
    if (sw_mode == 1) {
        pedal_pico_chorus_delay_time = PEDAL_PICO_CHORUS_DELAY_TIME_FIXED_1;
    } else if (sw_mode == 2) {
        pedal_pico_chorus_delay_time = PEDAL_PICO_CHORUS_DELAY_TIME_FIXED_3;
    } else {
        pedal_pico_chorus_delay_time = PEDAL_PICO_CHORUS_DELAY_TIME_FIXED_2;
    }
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    /* Push and Pop Delay */
    pedal_pico_chorus_delay_array[pedal_pico_chorus_delay_index] = (int16)normalized_1; // Push Current Value in Advance for 0
    int32 delay_1 = (int32)pedal_pico_chorus_delay_array[((pedal_pico_chorus_delay_index + PEDAL_PICO_CHORUS_DELAY_TIME_MAX) - pedal_pico_chorus_delay_time) % PEDAL_PICO_CHORUS_DELAY_TIME_MAX];
    pedal_pico_chorus_delay_index++;
    if (pedal_pico_chorus_delay_index >= PEDAL_PICO_CHORUS_DELAY_TIME_MAX) pedal_pico_chorus_delay_index -= PEDAL_PICO_CHORUS_DELAY_TIME_MAX;
    /* Get Oscillator */
    int32 fixed_point_value_sine_1 = util_pedal_pico_table_sine_1[pedal_pico_chorus_osc_sine_1_index / PEDAL_PICO_CHORUS_OSC_SINE_1_TIME_MULTIPLIER];
    pedal_pico_chorus_osc_sine_1_index += pedal_pico_chorus_osc_speed;
    if (pedal_pico_chorus_osc_sine_1_index >= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_CHORUS_OSC_SINE_1_TIME_MULTIPLIER) pedal_pico_chorus_osc_sine_1_index -= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_CHORUS_OSC_SINE_1_TIME_MULTIPLIER;
    delay_1 = (int32)(int64)((((int64)delay_1 << 16) * (int64)pedal_pico_chorus_delay_amplitude) >> 32);
    int32 delay_1_l = (int32)(int64)((((int64)delay_1 << 16) * (int64)abs(fixed_point_value_sine_1)) >> 32);
    int32 delay_1_r = (int32)(int64)((((int64)delay_1 << 16) * (int64)(0x00010000 - abs(fixed_point_value_sine_1))) >> 32);
    /* Push and Pop Distance */
    pedal_pico_chorus_lr_distance_array[pedal_pico_chorus_lr_distance_index] = (int16)((normalized_1 + delay_1_r) >> 1); // Push Current Value in Advance for 0
    int32 lr_distance_1 = (int32)pedal_pico_chorus_lr_distance_array[((pedal_pico_chorus_lr_distance_index + PEDAL_PICO_CHORUS_LR_DISTANCE_TIME_MAX) - pedal_pico_chorus_lr_distance_time_interpolation) % PEDAL_PICO_CHORUS_LR_DISTANCE_TIME_MAX];
    pedal_pico_chorus_lr_distance_index++;
    if (pedal_pico_chorus_lr_distance_index >= PEDAL_PICO_CHORUS_LR_DISTANCE_TIME_MAX) pedal_pico_chorus_lr_distance_index -= PEDAL_PICO_CHORUS_LR_DISTANCE_TIME_MAX;
    /* Output */
    pedal_pico_chorus->output_1 = ((normalized_1 + delay_1_l) >> 1);
    pedal_pico_chorus->output_1_inverted = -lr_distance_1;
}

void pedal_pico_chorus_free() { // Free Except Object, pedal_pico_chorus
    free((void*)pedal_pico_chorus_delay_array);
    free((void*)pedal_pico_chorus_lr_distance_array);
    __dsb();
}
