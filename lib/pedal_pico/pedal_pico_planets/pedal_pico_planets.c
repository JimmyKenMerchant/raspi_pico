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
    pedal_pico_planets_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_planets_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_planets_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    int32 coefficient = ((pedal_pico_planets_conversion_2 >> 7) + 1) << PEDAL_PICO_PLANETS_COEFFICIENT_SHIFT; // Make 5-bit Value (1-32) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    pedal_pico_planets_coefficient = coefficient;
    pedal_pico_planets_coefficient_interpolation = coefficient;
    pedal_pico_planets_delay_x = (int16*)calloc(PEDAL_PICO_PLANETS_DELAY_TIME_MAX, sizeof(int16));
    pedal_pico_planets_delay_y = (int16*)calloc(PEDAL_PICO_PLANETS_DELAY_TIME_MAX, sizeof(int16));
    uint16 delay_time = ((pedal_pico_planets_conversion_3 >> 7) + 1) << PEDAL_PICO_PLANETS_DELAY_TIME_SHIFT; // Make 5-bit Value (1-32) and Shift
    pedal_pico_planets_delay_time = delay_time;
    pedal_pico_planets_delay_time_interpolation = delay_time;
    pedal_pico_planets_delay_index = 0;
}

void pedal_pico_planets_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    pedal_pico_planets_conversion_1 = conversion_1;
    if (abs(conversion_2 - pedal_pico_planets_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_planets_conversion_2 = conversion_2;
        pedal_pico_planets_coefficient = ((pedal_pico_planets_conversion_2 >> 7) + 1) << PEDAL_PICO_PLANETS_COEFFICIENT_SHIFT; // Make 5-bit Value (1-32) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    }
    if (abs(conversion_3 - pedal_pico_planets_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_planets_conversion_3 = conversion_3;
        pedal_pico_planets_delay_time = ((pedal_pico_planets_conversion_3 >> 7) + 1) << PEDAL_PICO_PLANETS_DELAY_TIME_SHIFT; // Make 5-bit Value (1-32) and Shift
    }
    pedal_pico_planets_coefficient_interpolation = util_pedal_pico_interpolate(pedal_pico_planets_coefficient_interpolation, pedal_pico_planets_coefficient, PEDAL_PICO_PLANETS_COEFFICIENT_INTERPOLATION_ACCUM);
    pedal_pico_planets_delay_time_interpolation = util_pedal_pico_interpolate(pedal_pico_planets_delay_time_interpolation, pedal_pico_planets_delay_time, PEDAL_PICO_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_1);
    int32 normalized_1 = (int32)pedal_pico_planets_conversion_1 - (int32)util_pedal_pico_adc_middle_moving_average;
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_planets_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, UTIL_PEDAL_PICO_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    int16 delay_x = pedal_pico_planets_delay_x[((pedal_pico_planets_delay_index + PEDAL_PICO_PLANETS_DELAY_TIME_MAX) - (uint16)((int16)pedal_pico_planets_delay_time_interpolation)) % PEDAL_PICO_PLANETS_DELAY_TIME_MAX];
    int16 delay_y = pedal_pico_planets_delay_y[((pedal_pico_planets_delay_index + PEDAL_PICO_PLANETS_DELAY_TIME_MAX) - (uint16)((int16)pedal_pico_planets_delay_time_interpolation)) % PEDAL_PICO_PLANETS_DELAY_TIME_MAX];
    /* First Stage: High Pass Filter and Correction */
    int32 high_pass_1 = (int32)((int64)((((int64)delay_x << 16) * -(int64)pedal_pico_planets_coefficient_interpolation) + (((int64)normalized_1 << 16) * (int64)(0x00010000 - pedal_pico_planets_coefficient_interpolation))) >> 32);
    high_pass_1 = (int32)(int64)((((int64)high_pass_1 << 16) * (int64)pedal_pico_planets_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(high_pass_1, UTIL_PEDAL_PICO_PWM_PEAK))]) >> 32);
    /* Second Stage: Low Pass Filter to Sound from First Stage */
    if (util_pedal_pico_sw_mode == 1) high_pass_1 = normalized_1; // Bypass High Pass Filter
    int32 low_pass_1 = (int32)((int64)((((int64)delay_y << 16) * (int64)pedal_pico_planets_coefficient_interpolation) + (((int64)high_pass_1 << 16) * (int64)(0x00010000 - pedal_pico_planets_coefficient_interpolation))) >> 32);
    pedal_pico_planets_delay_x[pedal_pico_planets_delay_index] = (int16)high_pass_1;
    pedal_pico_planets_delay_y[pedal_pico_planets_delay_index] = (int16)low_pass_1;
    pedal_pico_planets_delay_index++;
    if (pedal_pico_planets_delay_index >= PEDAL_PICO_PLANETS_DELAY_TIME_MAX) pedal_pico_planets_delay_index -= PEDAL_PICO_PLANETS_DELAY_TIME_MAX;
    int32 mixed_1;
    if (sw_mode == 1) {
        mixed_1 = low_pass_1;
    } else if (sw_mode == 2) {
        mixed_1 = high_pass_1;
    } else {
        mixed_1 = low_pass_1;
    }
    /* Output */
    pedal_pico_planets->output_1 = util_pedal_pico_cutoff_biased(mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, UTIL_PEDAL_PICO_PWM_OFFSET + UTIL_PEDAL_PICO_PWM_PEAK, UTIL_PEDAL_PICO_PWM_OFFSET - UTIL_PEDAL_PICO_PWM_PEAK);
    pedal_pico_planets->output_1_inverted = util_pedal_pico_cutoff_biased(-mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, UTIL_PEDAL_PICO_PWM_OFFSET + UTIL_PEDAL_PICO_PWM_PEAK, UTIL_PEDAL_PICO_PWM_OFFSET - UTIL_PEDAL_PICO_PWM_PEAK);
}

void pedal_pico_planets_free() { // Free Except Object, pedal_pico_planets
    free((void*)pedal_pico_planets_delay_x);
    free((void*)pedal_pico_planets_delay_y);
    __dsb();
}
