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

#include "pedal_pico/pedal_pico_phaser.h"

void pedal_pico_phaser_set() {
    if (! pedal_pico_phaser) panic("pedal_pico_phaser is not initialized.");
    pedal_pico_phaser_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_phaser_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_phaser_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_phaser_coefficient_swing = PEDAL_PICO_PHASER_COEFFICIENT_SWING_PEAK_FIXED_1;
    pedal_pico_phaser_delay_x_1 = (int16*)calloc(PEDAL_PICO_PHASER_DELAY_TIME_MAX, sizeof(int16));
    pedal_pico_phaser_delay_y_1 = (int16*)calloc(PEDAL_PICO_PHASER_DELAY_TIME_MAX, sizeof(int16));
    pedal_pico_phaser_delay_x_2 = (int16*)calloc(PEDAL_PICO_PHASER_DELAY_TIME_MAX, sizeof(int16));
    pedal_pico_phaser_delay_y_2 = (int16*)calloc(PEDAL_PICO_PHASER_DELAY_TIME_MAX, sizeof(int16));
    pedal_pico_phaser_delay_time = PEDAL_PICO_PHASER_DELAY_TIME_FIXED_1;
    pedal_pico_phaser_delay_index = 0;
    pedal_pico_phaser_osc_sine_1_index = 0;
    pedal_pico_phaser_osc_speed = pedal_pico_phaser_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT; // Make 5-bit Value (0-31)
    pedal_pico_phaser_osc_start_threshold = (pedal_pico_phaser_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) * PEDAL_PICO_PHASER_OSC_START_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    pedal_pico_phaser_osc_start_count = 0;
}

void pedal_pico_phaser_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    pedal_pico_phaser_conversion_1 = conversion_1;
    if (abs(conversion_2 - pedal_pico_phaser_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_phaser_conversion_2 = conversion_2;
        pedal_pico_phaser_osc_speed = pedal_pico_phaser_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT; // Make 5-bit Value (0-31)
    }
    if (abs(conversion_3 - pedal_pico_phaser_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_phaser_conversion_3 = conversion_3;
        pedal_pico_phaser_osc_start_threshold = (pedal_pico_phaser_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) * PEDAL_PICO_PHASER_OSC_START_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    }
    int32 normalized_1 = (int32)pedal_pico_phaser_conversion_1 - (int32)util_pedal_pico_adc_middle_moving_average;
    if (sw_mode == 1) {
        pedal_pico_phaser_delay_time = PEDAL_PICO_PHASER_DELAY_TIME_FIXED_1;
    } else if (sw_mode == 2) {
        pedal_pico_phaser_delay_time = PEDAL_PICO_PHASER_DELAY_TIME_FIXED_3;
    } else {
        pedal_pico_phaser_delay_time = PEDAL_PICO_PHASER_DELAY_TIME_FIXED_2;
    }
    /**
     * pedal_pico_phaser_osc_start_count:
     *
     * Over Positive Threshold       ## 1
     *-----------------------------------------------------------------------------------------------------------
     * Under Positive Threshold     # 0 # 2      ### Reset to 1
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                  # 0   # 3   # 5   # 2
     *-----------------------------------------------------------------------------------------------------------
     * 0                           # 0   # 4   # 4   # 3   # 5 ...Count Up to PEDAL_PICO_PHASER_OSC_START_COUNT_MAX
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                         # 5 # 3      #### 4
     *-----------------------------------------------------------------------------------------------------------
     * Under Negative Threshold           # 6 # 2
     *-----------------------------------------------------------------------------------------------------------
     * Over Negative Threshold             ## Reset to 1
     */
    if (normalized_1 > pedal_pico_phaser_osc_start_threshold || normalized_1 < -pedal_pico_phaser_osc_start_threshold) {
        pedal_pico_phaser_osc_start_count = 1;
    } else if (pedal_pico_phaser_osc_start_count != 0 && (normalized_1 > (pedal_pico_phaser_osc_start_threshold >> 1) || normalized_1 < -(pedal_pico_phaser_osc_start_threshold >> 1))) {
        pedal_pico_phaser_osc_start_count = 1;
    } else if (pedal_pico_phaser_osc_start_count != 0) {
        pedal_pico_phaser_osc_start_count++;
    }
    if (pedal_pico_phaser_osc_start_count >= PEDAL_PICO_PHASER_OSC_START_COUNT_MAX) pedal_pico_phaser_osc_start_count = 0;
    if (pedal_pico_phaser_osc_start_count == 0) {
        pedal_pico_phaser_osc_sine_1_index = 0;
    }
    /* Get Oscillator */
    int32 fixed_point_value_sine_1 = util_pedal_pico_table_sine_1[pedal_pico_phaser_osc_sine_1_index / PEDAL_PICO_PHASER_OSC_SINE_1_TIME_MULTIPLIER];
    pedal_pico_phaser_osc_sine_1_index += pedal_pico_phaser_osc_speed;
    if (pedal_pico_phaser_osc_sine_1_index >= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_PHASER_OSC_SINE_1_TIME_MULTIPLIER) pedal_pico_phaser_osc_sine_1_index -= (UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_PHASER_OSC_SINE_1_TIME_MULTIPLIER);
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
     normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)util_pedal_pico_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, UTIL_PEDAL_PICO_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    /**
     * Phaser is the synthesis of the concurrent wave and the phase shifted concurrent wave.
     * The phase shifted concurrent wave is made by an all-pass filter.
     *
     * Transfer Function (Z Transform) of All-Pass Filter:
     * H[Z] = frac{Y[Z]}{X[Z]} = frac{K + Z^-1}{1 + K*Z^-1}: Where Z is e^(j*omega*T) = complex number, K is coefficient (-1 <= K <= 1).
     * Y[Z] = X[Z]*Z^-1 - K*Y[Z]*Z^-1 + K*X[Z]
     * In the inverse Z transform to the differential equation for the discreate-time signal,
     * X[Z]*Z^-1 becomes X[S - 1], and Y[Z]*Z^-1 becomes Y[S - 1], where S is the current sample.
     * Y[S] = X[S - 1] - K*Y[S - 1] + K*X[S]: I use this.
     * Y[S] = X[S - 1] + K'*Y[S - 1] - K'*X[S]: Where K' = -K.
     *
     * All-pass filter is the combination of the low-pass filter and the high-pass filter on the same stage.
     * Low-pass Filter: Y[S] = K''*Y[S - 1] + (1 - K'')*X[S]: It's like a speaker in a closed box.
     * High-pass Filter: Y[S] = K''*X[S - 1] + (1 - K'')*X[S]: It's like a speaker with a reflection board.
     * All-pass Filter: Low-pass Filter + High-pass Filter - X[S]: K''*Y[S - 1] + K''*X[S - 1] + X[S] - 2*K''*X[S]
     * All-pass Filter with K'' = 1: X[S - 1] + Y[S - 1] - X[S]: With unknown K' = 1, this function actually makes a phase shift.
     * All-pass Filter Y(S) with S{5,-5,5,-5} where K' = 1 and preceding Y(S - 1) = 0 results Y{-5,5,-5,5}. This means phase shift 180 degrees delay.
     * X[S - 1] and Y[S - 1] are effected on a high frequency.
     * To get the effect on intended frequencies, use X[S - N] and Y[S - N] where N is the number of delay.
     * If the sampling frequency is 30518Hz, the effective frequency is its Nyquist (or folding) frequency and over, i.e., 15259Hz <=.
     * In this case, the most effective frequency is 15259Hz, and other frequencies far from the frequency isn't effected.
     * This phenomenon can also use for canceling noise at the intended frequency.
     */
    /* First Stage All-pass Filter for Noise Cancel */
    int16 delay_x_1 = pedal_pico_phaser_delay_x_1[((pedal_pico_phaser_delay_index + PEDAL_PICO_PHASER_DELAY_TIME_MAX) - pedal_pico_phaser_delay_time) % PEDAL_PICO_PHASER_DELAY_TIME_MAX];
    int16 delay_y_1 = pedal_pico_phaser_delay_y_1[((pedal_pico_phaser_delay_index + PEDAL_PICO_PHASER_DELAY_TIME_MAX) - pedal_pico_phaser_delay_time) % PEDAL_PICO_PHASER_DELAY_TIME_MAX];
    //if (pedal_pico_phaser_delay_time) delay_x = 0; // No Delay, Otherwise Latest
    int32 phase_shift_1 = (int32)((int64)(((int64)delay_x_1 << 32) - ((int64)delay_y_1 << 32) + ((int64)normalized_1 << 32)) >> 32); // Coefficient = 1
    int32 canceled_1 = (normalized_1 + phase_shift_1) >> 1;
    pedal_pico_phaser_delay_x_1[pedal_pico_phaser_delay_index] = (int16)normalized_1;
    pedal_pico_phaser_delay_y_1[pedal_pico_phaser_delay_index] = (int16)phase_shift_1;
    /* Second Stage All-pass Filter for Phaser */
    int16 delay_x_2 = pedal_pico_phaser_delay_x_2[((pedal_pico_phaser_delay_index + PEDAL_PICO_PHASER_DELAY_TIME_MAX) - pedal_pico_phaser_delay_time) % PEDAL_PICO_PHASER_DELAY_TIME_MAX];
    int16 delay_y_2 = pedal_pico_phaser_delay_y_2[((pedal_pico_phaser_delay_index + PEDAL_PICO_PHASER_DELAY_TIME_MAX) - pedal_pico_phaser_delay_time) % PEDAL_PICO_PHASER_DELAY_TIME_MAX];
    int32 coefficient = (int32)(int64)(((int64)pedal_pico_phaser_coefficient_swing * (int64)fixed_point_value_sine_1) >> 16); // Remain Decimal Part
    int32 phase_shift_2 = (int32)((int64)(((int64)delay_x_2 << 32) - (((int64)delay_y_2 << 16) * (int64)coefficient) + (((int64)canceled_1 << 16) * (int64)coefficient)) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    pedal_pico_phaser_delay_x_2[pedal_pico_phaser_delay_index] = (int16)canceled_1;
    pedal_pico_phaser_delay_y_2[pedal_pico_phaser_delay_index] = (int16)phase_shift_2;
    pedal_pico_phaser_delay_index++;
    if (pedal_pico_phaser_delay_index >= PEDAL_PICO_PHASER_DELAY_TIME_MAX) pedal_pico_phaser_delay_index -= PEDAL_PICO_PHASER_DELAY_TIME_MAX;
    int32 mixed_1 = (canceled_1 - phase_shift_2) >> 1;
    pedal_pico_phaser->output_1 = util_pedal_pico_cutoff_biased(mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, UTIL_PEDAL_PICO_PWM_OFFSET + UTIL_PEDAL_PICO_PWM_PEAK, UTIL_PEDAL_PICO_PWM_OFFSET - UTIL_PEDAL_PICO_PWM_PEAK);
    pedal_pico_phaser->output_1_inverted = util_pedal_pico_cutoff_biased(-mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, UTIL_PEDAL_PICO_PWM_OFFSET + UTIL_PEDAL_PICO_PWM_PEAK, UTIL_PEDAL_PICO_PWM_OFFSET - UTIL_PEDAL_PICO_PWM_PEAK);
}

void pedal_pico_phaser_free() { // Free Except Object, pedal_pico_phaser
    free((void*)pedal_pico_phaser_delay_x_1);
    free((void*)pedal_pico_phaser_delay_y_1);
    free((void*)pedal_pico_phaser_delay_x_2);
    free((void*)pedal_pico_phaser_delay_y_2);
    __dsb();
}
