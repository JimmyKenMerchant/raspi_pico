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

#include "pedal_pico/pedal_pico_sideband.h"

void pedal_pico_sideband_set() {
    if (! pedal_pico_sideband) panic("pedal_pico_sideband is not initialized.");
    pedal_pico_sideband_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_sideband_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_sideband_osc_sine_1_index = 0;
    pedal_pico_sideband_osc_sine_2_index = 0;
    pedal_pico_sideband_osc_speed = pedal_pico_sideband_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT; // Make 5-bit Value (0-31)
    pedal_pico_sideband_osc_start_threshold = (pedal_pico_sideband_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) * PEDAL_PICO_SIDEBAND_OSC_START_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    pedal_pico_sideband_osc_start_count = 0;
}

void pedal_pico_sideband_process(int32 normalized_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    if (abs(conversion_2 - pedal_pico_sideband_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_sideband_conversion_2 = conversion_2;
        pedal_pico_sideband_osc_speed = pedal_pico_sideband_conversion_2 >> UTIL_PEDAL_PICO_ADC_SHIFT; // Make 5-bit Value (0-31)
    }
    if (abs(conversion_3 - pedal_pico_sideband_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_sideband_conversion_3 = conversion_3;
        pedal_pico_sideband_osc_start_threshold = (pedal_pico_sideband_conversion_3 >> UTIL_PEDAL_PICO_ADC_SHIFT) * PEDAL_PICO_SIDEBAND_OSC_START_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    }
    /**
     * pedal_pico_sideband_osc_start_count:
     *
     * Over Positive Threshold       ## 1
     *-----------------------------------------------------------------------------------------------------------
     * Under Positive Threshold     # 0 # 2      ### Reset to 1
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                  # 0   # 3   # 5   # 2
     *-----------------------------------------------------------------------------------------------------------
     * 0                           # 0   # 4   # 4   # 3   # 5 ...Count Up to PEDAL_PICO_SIDEBAND_OSC_START_COUNT_MAX
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                         # 5 # 3      #### 4
     *-----------------------------------------------------------------------------------------------------------
     * Under Negative Threshold           # 6 # 2
     *-----------------------------------------------------------------------------------------------------------
     * Over Negative Threshold             ## Reset to 1
     */
    if (normalized_1 > pedal_pico_sideband_osc_start_threshold || normalized_1 < -pedal_pico_sideband_osc_start_threshold) {
        pedal_pico_sideband_osc_start_count = 1;
    } else if (pedal_pico_sideband_osc_start_count != 0 && (normalized_1 > (pedal_pico_sideband_osc_start_threshold >> 1) || normalized_1 < -(pedal_pico_sideband_osc_start_threshold >> 1))) {
        pedal_pico_sideband_osc_start_count = 1;
    } else if (pedal_pico_sideband_osc_start_count != 0) {
        pedal_pico_sideband_osc_start_count++;
    }
    if (pedal_pico_sideband_osc_start_count >= PEDAL_PICO_SIDEBAND_OSC_START_COUNT_MAX) pedal_pico_sideband_osc_start_count = 0;
    if (pedal_pico_sideband_osc_start_count == 0) {
        pedal_pico_sideband_osc_sine_1_index = 0;
        pedal_pico_sideband_osc_sine_2_index = 0;
    }
    normalized_1 = util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_SIDEBAND_CUTOFF_FIXED_1);
    int32 fixed_point_value_sine_1 = util_pedal_pico_table_sine_1[pedal_pico_sideband_osc_sine_1_index / PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER];
    int32 fixed_point_value_sine_2 = util_pedal_pico_table_sine_1[pedal_pico_sideband_osc_sine_2_index / PEDAL_PICO_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER] >> 1; // Divide By 2
    pedal_pico_sideband_osc_sine_1_index += pedal_pico_sideband_osc_speed;
    pedal_pico_sideband_osc_sine_2_index += pedal_pico_sideband_osc_speed;
    if (pedal_pico_sideband_osc_sine_1_index >= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER) pedal_pico_sideband_osc_sine_1_index -= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER;
    if (pedal_pico_sideband_osc_sine_2_index >= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER) pedal_pico_sideband_osc_sine_2_index -= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER;
    int32 osc_value = (int32)(int64)((((int64)PEDAL_PICO_SIDEBAND_OSC_PEAK << 16) * (((int64)fixed_point_value_sine_1 + (int64)fixed_point_value_sine_2)) >> 1) >> 16); // Remain Decimal Part
    osc_value = (int32)(int64)(((int64)osc_value * ((int64)abs(normalized_1) << PEDAL_PICO_SIDEBAND_GAIN_SHIFT_FIXED_1)) >> 32); // Absolute normalized_1 to Multiply Frequency
    osc_value -= PEDAL_PICO_SIDEBAND_OSC_PEAK >> 1;
    /* Output */
    pedal_pico_sideband->output_1 = osc_value;
    pedal_pico_sideband->output_1_inverted = -osc_value;
}

void pedal_pico_sideband_free() { // Free Except Object, pedal_pico_sideband
    __dsb();
}
