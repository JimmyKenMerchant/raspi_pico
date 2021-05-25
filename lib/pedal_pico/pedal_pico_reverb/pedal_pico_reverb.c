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
    pedal_pico_reverb_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_reverb_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_reverb_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_reverb_delay_array = (int16*)calloc(PEDAL_PICO_REVERB_DELAY_TIME_MAX, sizeof(int16));
    pedal_pico_reverb_delay_amplitude = (int32)(pedal_pico_reverb_conversion_2 >> 7) << PEDAL_PICO_REVERB_DELAY_AMPLITUDE_SHIFT; // Make 5-bit Value (0-31) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    uint16 delay_time = (pedal_pico_reverb_conversion_3 >> 7) << PEDAL_PICO_REVERB_DELAY_TIME_SHIFT; // Make 5-bit Value (0-31) and Multiply
    pedal_pico_reverb_delay_time = delay_time;
    pedal_pico_reverb_delay_time_interpolation = delay_time;
    pedal_pico_reverb_delay_index = 0;
}

void pedal_pico_reverb_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    pedal_pico_reverb_conversion_1 = conversion_1;
    if (abs(conversion_2 - pedal_pico_reverb_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_reverb_conversion_2 = conversion_2;
        pedal_pico_reverb_delay_amplitude = (int32)(pedal_pico_reverb_conversion_2 >> 7) << PEDAL_PICO_REVERB_DELAY_AMPLITUDE_SHIFT; // Make 5-bit Value (0-31) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    }
    if (abs(conversion_3 - pedal_pico_reverb_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_reverb_conversion_3 = conversion_3;
        pedal_pico_reverb_delay_time = (pedal_pico_reverb_conversion_3 >> 7) << PEDAL_PICO_REVERB_DELAY_TIME_SHIFT; // Make 5-bit Value (0-31) and Multiply Multiply
    }
    pedal_pico_reverb_delay_time_interpolation = util_pedal_pico_interpolate(pedal_pico_reverb_delay_time_interpolation, pedal_pico_reverb_delay_time, PEDAL_PICO_REVERB_DELAY_TIME_INTERPOLATION_ACCUM);
    int32 normalized_1 = (int32)pedal_pico_reverb_conversion_1 - (int32)util_pedal_pico_adc_middle_moving_average;
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)util_pedal_pico_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, UTIL_PEDAL_PICO_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    int32 delay_1 = (int32)pedal_pico_reverb_delay_array[((pedal_pico_reverb_delay_index + PEDAL_PICO_REVERB_DELAY_TIME_MAX) - pedal_pico_reverb_delay_time_interpolation) % PEDAL_PICO_REVERB_DELAY_TIME_MAX];
    if (pedal_pico_reverb_delay_time_interpolation == 0) delay_1 = 0; // No Reverb, Otherwise Latest
    int32 pedal_pico_reverb_normalized_1_amplitude = 0x00010000 - pedal_pico_reverb_delay_amplitude;
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_reverb_normalized_1_amplitude) >> 32);
    delay_1 = (int32)(int64)((((int64)delay_1 << 16) * (int64)pedal_pico_reverb_delay_amplitude) >> 32);
    int32 mixed_1 = normalized_1 + delay_1;
    pedal_pico_reverb_delay_array[pedal_pico_reverb_delay_index] = (int16)mixed_1;
    pedal_pico_reverb_delay_index++;
    if (pedal_pico_reverb_delay_index >= PEDAL_PICO_REVERB_DELAY_TIME_MAX) pedal_pico_reverb_delay_index -= PEDAL_PICO_REVERB_DELAY_TIME_MAX;
    /* Output */
    pedal_pico_reverb->output_1 = util_pedal_pico_cutoff_biased(mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, UTIL_PEDAL_PICO_PWM_OFFSET + UTIL_PEDAL_PICO_PWM_PEAK, UTIL_PEDAL_PICO_PWM_OFFSET - UTIL_PEDAL_PICO_PWM_PEAK);
    pedal_pico_reverb->output_1_inverted = util_pedal_pico_cutoff_biased(-mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, UTIL_PEDAL_PICO_PWM_OFFSET + UTIL_PEDAL_PICO_PWM_PEAK, UTIL_PEDAL_PICO_PWM_OFFSET - UTIL_PEDAL_PICO_PWM_PEAK);
}

void pedal_pico_reverb_free() { // Free Except Object, pedal_pico_reverb
    free((void*)pedal_pico_reverb_delay_array);
    __dsb();
}
