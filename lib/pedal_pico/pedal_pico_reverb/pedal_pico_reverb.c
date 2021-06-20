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

#include "pedal_pico/pedal_pico_reverb.h"

void pedal_pico_reverb_set() {
    if (! pedal_pico_reverb) panic("pedal_pico_reverb is not initialized.");
    pedal_pico_reverb_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_reverb_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_reverb_delay_array = (int16*)calloc(PEDAL_PICO_REVERB_DELAY_TIME_MAX, sizeof(int16));
    pedal_pico_reverb_delay_amplitude = (int32)(pedal_pico_reverb_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT) << PEDAL_PICO_REVERB_DELAY_AMPLITUDE_SHIFT; // Make 5-bit Value (0-31) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    uint16 delay_time = (pedal_pico_reverb_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) << PEDAL_PICO_REVERB_DELAY_TIME_SHIFT; // Make 5-bit Value (0-31) and Multiply
    pedal_pico_reverb_delay_time = delay_time;
    pedal_pico_reverb_delay_time_interpolation = delay_time;
    pedal_pico_reverb_delay_index = 0;
}

void pedal_pico_reverb_process(int32 normalized_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    if (abs(conversion_2 - pedal_pico_reverb_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_reverb_conversion_2 = conversion_2;
        pedal_pico_reverb_delay_amplitude = (int32)(pedal_pico_reverb_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT) << PEDAL_PICO_REVERB_DELAY_AMPLITUDE_SHIFT; // Make 5-bit Value (0-31) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    }
    if (abs(conversion_3 - pedal_pico_reverb_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_reverb_conversion_3 = conversion_3;
        pedal_pico_reverb_delay_time = (pedal_pico_reverb_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) << PEDAL_PICO_REVERB_DELAY_TIME_SHIFT; // Make 5-bit Value (0-31) and Multiply Multiply
    }
    pedal_pico_reverb_delay_time_interpolation = _interpolate(pedal_pico_reverb_delay_time_interpolation, pedal_pico_reverb_delay_time, PEDAL_PICO_REVERB_DELAY_TIME_INTERPOLATION_ACCUM);
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    int32 delay_1 = (int32)pedal_pico_reverb_delay_array[((pedal_pico_reverb_delay_index + PEDAL_PICO_REVERB_DELAY_TIME_MAX) - pedal_pico_reverb_delay_time_interpolation) % PEDAL_PICO_REVERB_DELAY_TIME_MAX];
    if (pedal_pico_reverb_delay_time_interpolation == 0) delay_1 = 0; // No Reverb, Otherwise Latest
    int32 pedal_pico_reverb_normalized_1_amplitude = 0x00010000 - pedal_pico_reverb_delay_amplitude;
    int32 reduced_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_reverb_normalized_1_amplitude) >> 32);
    delay_1 = (int32)(int64)((((int64)delay_1 << 16) * (int64)pedal_pico_reverb_delay_amplitude) >> 32);
    int32 mixed_1 = reduced_1 + delay_1;
    if (sw_mode == 1) {
        pedal_pico_reverb_delay_array[pedal_pico_reverb_delay_index] = (int16)normalized_1;
    } else {
        pedal_pico_reverb_delay_array[pedal_pico_reverb_delay_index] = (int16)mixed_1;
    }
    pedal_pico_reverb_delay_index++;
    if (pedal_pico_reverb_delay_index >= PEDAL_PICO_REVERB_DELAY_TIME_MAX) pedal_pico_reverb_delay_index -= PEDAL_PICO_REVERB_DELAY_TIME_MAX;
    /* Output */
    pedal_pico_reverb->output_1 = mixed_1;
    pedal_pico_reverb->output_1_inverted = -mixed_1;
}

void pedal_pico_reverb_free() { // Free Except Object, pedal_pico_reverb
    free((void*)pedal_pico_reverb_delay_array);
    __dsb();
}
