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

#include "pedal_pico/pedal_pico_envelope.h"

void pedal_pico_envelope_set() {
    if (! pedal_pico_envelope) panic("pedal_pico_envelope is not initialized.");
    pedal_pico_envelope_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_envelope_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_envelope_decay_triangle_1_index = 0;
    pedal_pico_envelope_decay_speed = (pedal_pico_envelope_conversion_2 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT) + 1; // Make 6-bit Value (1-64)
    pedal_pico_envelope_decay_is_release = false;
    pedal_pico_envelope_decay_start_threshold = (pedal_pico_envelope_conversion_3 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT) * PEDAL_PICO_ENVELOPE_DECAY_THRESHOLD_MULTIPLIER; // Make 6-bit Value (0-63) and Multiply
    pedal_pico_envelope_decay_start_count = 0;
    pedal_pico_envelope_decay_is_outgoing = false;
}

void pedal_pico_envelope_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (abs(conversion_2 - pedal_pico_envelope_conversion_2) >= UTIL_PEDAL_PICO_ADC_FINE_THRESHOLD) {
        pedal_pico_envelope_conversion_2 = conversion_2;
        pedal_pico_envelope_decay_speed = (pedal_pico_envelope_conversion_2 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT) + 1; // Make 6-bit Value (1-64)
    }
    if (abs(conversion_3 - pedal_pico_envelope_conversion_3) >= UTIL_PEDAL_PICO_ADC_FINE_THRESHOLD) {
        pedal_pico_envelope_conversion_3 = conversion_3;
        pedal_pico_envelope_decay_start_threshold = (pedal_pico_envelope_conversion_3 >> UTIL_PEDAL_PICO_ADC_FINE_SHIFT) * PEDAL_PICO_ENVELOPE_DECAY_THRESHOLD_MULTIPLIER; // Make 6-bit Value (0-63) and Multiply
    }
    /* Decay Start */
    pedal_pico_envelope_decay_start_count = util_pedal_pico_threshold_gate_count(pedal_pico_envelope_decay_start_count, normalized_1, pedal_pico_envelope_decay_start_threshold, PEDAL_PICO_ENVELOPE_DECAY_HYSTERESIS_SHIFT);
    if (pedal_pico_envelope_decay_start_count >= PEDAL_PICO_ENVELOPE_DECAY_COUNT_MAX) pedal_pico_envelope_decay_start_count = 0;
    if (pedal_pico_envelope_decay_start_count == 1 && ! pedal_pico_envelope_decay_is_outgoing) {
            pedal_pico_envelope_decay_triangle_1_index = 0;
            pedal_pico_envelope_decay_is_release = false;
            pedal_pico_envelope_decay_is_outgoing = true;
    }
    if (pedal_pico_envelope_decay_is_outgoing) {
        /* Decay or Release */
        int32_t fixed_point_value_triangle_1 = util_pedal_pico_table_triangle_1[abs(pedal_pico_envelope_decay_triangle_1_index) / PEDAL_PICO_ENVELOPE_DECAY_TRIANGLE_1_TIME_MULTIPLIER]; // Depending on decay_speed, the index value may have a negative value.
        pedal_pico_envelope_decay_is_release ? (pedal_pico_envelope_decay_triangle_1_index -= pedal_pico_envelope_decay_speed) : (pedal_pico_envelope_decay_triangle_1_index += pedal_pico_envelope_decay_speed); // Check Decay or Release
        if (pedal_pico_envelope_decay_triangle_1_index >= UTIL_PEDAL_PICO_OSC_TRIANGLE_1_TIME_MAX * PEDAL_PICO_ENVELOPE_DECAY_TRIANGLE_1_TIME_MULTIPLIER) { // End of Decay
            pedal_pico_envelope_decay_triangle_1_index = (UTIL_PEDAL_PICO_OSC_TRIANGLE_1_TIME_MAX * PEDAL_PICO_ENVELOPE_DECAY_TRIANGLE_1_TIME_MULTIPLIER) - 1;
            pedal_pico_envelope_decay_is_release = true;
        } else if (pedal_pico_envelope_decay_triangle_1_index <= 0) { // End of Release
            pedal_pico_envelope_decay_triangle_1_index = 0;
            pedal_pico_envelope_decay_is_release = false;
            pedal_pico_envelope_decay_is_outgoing = false;
        }
        /**
         * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
         * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
         * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
         */
        if (sw_mode == 1) {
            if (fixed_point_value_triangle_1 >= PEDAL_PICO_ENVELOPE_DECAY_LIMIT_FIXED_1) {
                fixed_point_value_triangle_1 = PEDAL_PICO_ENVELOPE_DECAY_LIMIT_FIXED_1;
            }
        } else if (sw_mode == 2) {
            if (fixed_point_value_triangle_1 >= PEDAL_PICO_ENVELOPE_DECAY_LIMIT_FIXED_3) {
                fixed_point_value_triangle_1 = PEDAL_PICO_ENVELOPE_DECAY_LIMIT_FIXED_3;
            }
        } else {
            if (fixed_point_value_triangle_1 >= PEDAL_PICO_ENVELOPE_DECAY_LIMIT_FIXED_2) {
                fixed_point_value_triangle_1 = PEDAL_PICO_ENVELOPE_DECAY_LIMIT_FIXED_2;
            }
        }
        normalized_1 = (int32_t)((((int64_t)normalized_1 << 16) * (int64_t)(0x00010000 - fixed_point_value_triangle_1)) >> 32);
    }
    pedal_pico_envelope->output_1 = normalized_1;
    pedal_pico_envelope->output_1_inverted = -normalized_1;
}

void pedal_pico_envelope_free() { // Free Except Object, pedal_pico_envelope
    __dsb();
}
