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
    pedal_pico_sideband_osc_speed = pedal_pico_sideband_conversion_2 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT; // Make 5-bit Value (0-31)
    pedal_pico_sideband_osc_start_threshold = (pedal_pico_sideband_conversion_3 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT) * PEDAL_PICO_SIDEBAND_OSC_START_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    pedal_pico_sideband_osc_start_count = 0;
    pedal_pico_sideband_middle_moving_average_sum = (PEDAL_PICO_SIDEBAND_OSC_PEAK >> 1) * PEDAL_PICO_SIDEBAND_MIDDLE_MOVING_AVERAGE_NUMBER;
    pedal_pico_sideband_wave_moving_average_sum = 0;
}

void pedal_pico_sideband_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (abs(conversion_2 - pedal_pico_sideband_conversion_2) > UTIL_PEDAL_PICO_ADC_FINE_THRESHOLD) {
        pedal_pico_sideband_conversion_2 = conversion_2;
        pedal_pico_sideband_osc_speed = pedal_pico_sideband_conversion_2 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT; // Make 5-bit Value (0-31)
    }
    if (abs(conversion_3 - pedal_pico_sideband_conversion_3) > UTIL_PEDAL_PICO_ADC_FINE_THRESHOLD) {
        pedal_pico_sideband_conversion_3 = conversion_3;
        pedal_pico_sideband_osc_start_threshold = (pedal_pico_sideband_conversion_3 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT) * PEDAL_PICO_SIDEBAND_OSC_START_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    }
    /* Oscillator Start */
    pedal_pico_sideband_osc_start_count = util_pedal_pico_threshold_gate_count(pedal_pico_sideband_osc_start_count, normalized_1, pedal_pico_sideband_osc_start_threshold, PEDAL_PICO_SIDEBAND_OSC_START_HYSTERESIS_SHIFT);
    if (pedal_pico_sideband_osc_start_count >= PEDAL_PICO_SIDEBAND_OSC_START_COUNT_MAX) pedal_pico_sideband_osc_start_count = 0;
    if (pedal_pico_sideband_osc_start_count == 0) {
        pedal_pico_sideband_osc_sine_1_index = 0;
        pedal_pico_sideband_osc_sine_2_index = 0;
    }
    normalized_1 = util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_SIDEBAND_CUTOFF_FIXED_1);
    int32_t fixed_point_value_sine_1 = util_pedal_pico_table_sine_1[pedal_pico_sideband_osc_sine_1_index / PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER];
    int32_t fixed_point_value_sine_2 = util_pedal_pico_table_sine_1[pedal_pico_sideband_osc_sine_2_index / PEDAL_PICO_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER] >> 1; // Divide By 2
    pedal_pico_sideband_osc_sine_1_index += pedal_pico_sideband_osc_speed;
    pedal_pico_sideband_osc_sine_2_index += pedal_pico_sideband_osc_speed;
    if (pedal_pico_sideband_osc_sine_1_index >= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER) pedal_pico_sideband_osc_sine_1_index -= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER;
    if (pedal_pico_sideband_osc_sine_2_index >= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER) pedal_pico_sideband_osc_sine_2_index -= UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX * PEDAL_PICO_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER;
    int32_t osc_value = (int32_t)((((int64_t)PEDAL_PICO_SIDEBAND_OSC_PEAK << 16) * (((int64_t)fixed_point_value_sine_1 + (int64_t)fixed_point_value_sine_2) >> 1)) >> 16); // Remain Decimal Part
    if (sw_mode == 1) {
        osc_value = abs(normalized_1) << 1;
    } else {
        osc_value = (int32_t)(((int64_t)osc_value * ((int64_t)abs(normalized_1) << PEDAL_PICO_SIDEBAND_GAIN_SHIFT_FIXED_1)) >> 32); // Absolute normalized_1 to Multiply Frequency
    }
    /* Correction of Biasing */
    int32_t middle_moving_average = pedal_pico_sideband_middle_moving_average_sum / PEDAL_PICO_SIDEBAND_MIDDLE_MOVING_AVERAGE_NUMBER;
    pedal_pico_sideband_middle_moving_average_sum -= middle_moving_average;
    pedal_pico_sideband_middle_moving_average_sum += osc_value;
    middle_moving_average = pedal_pico_sideband_middle_moving_average_sum / PEDAL_PICO_SIDEBAND_MIDDLE_MOVING_AVERAGE_NUMBER;
    osc_value -= middle_moving_average;
    if (sw_mode == 1) {
        /* Low-pass filter */
        int32_t wave_moving_average = pedal_pico_sideband_wave_moving_average_sum / PEDAL_PICO_SIDEBAND_WAVE_MOVING_AVERAGE_NUMBER;
        pedal_pico_sideband_wave_moving_average_sum -= wave_moving_average;
        pedal_pico_sideband_wave_moving_average_sum += osc_value;
        osc_value = pedal_pico_sideband_wave_moving_average_sum / PEDAL_PICO_SIDEBAND_WAVE_MOVING_AVERAGE_NUMBER;
        osc_value <<= PEDAL_PICO_SIDEBAND_WAVE_SHIFT;
    }
    /* Output */
    pedal_pico_sideband->output_1 = osc_value;
    pedal_pico_sideband->output_1_inverted = -osc_value;
}

void pedal_pico_sideband_free() { // Free Except Object, pedal_pico_sideband
    __dsb();
}
