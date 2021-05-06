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

void pedal_pico_sideband_start() {
    if (! pedal_pico_sideband) panic("pedal_pico_sideband is not initialized.");
    /* PWM Settings */
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_pico_sideband_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0xF0);
    pwm_set_chan_level(pedal_pico_sideband->pwm_1_slice, pedal_pico_sideband->pwm_1_channel, PEDAL_PICO_SIDEBAND_PWM_OFFSET);
    pwm_set_chan_level(pedal_pico_sideband->pwm_2_slice, pedal_pico_sideband->pwm_2_channel, PEDAL_PICO_SIDEBAND_PWM_OFFSET);
    /* Unique Settings */
    pedal_pico_sideband_set();
    /* Start */
    util_pedal_pico_start((util_pedal_pico*)pedal_pico_sideband);
    util_pedal_pico_sw_loop(PEDAL_PICO_SIDEBAND_SW_1_GPIO, PEDAL_PICO_SIDEBAND_SW_2_GPIO);
}

void pedal_pico_sideband_set() {
    if (! pedal_pico_sideband) panic("pedal_pico_sideband is not initialized.");
    pedal_pico_sideband_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_sideband_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_sideband_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_sideband_osc_sine_1_index = 0;
    pedal_pico_sideband_osc_sine_2_index = 0;
    pedal_pico_sideband_osc_amplitude = PEDAL_PICO_SIDEBAND_OSC_AMPLITUDE_PEAK;
    pedal_pico_sideband_osc_speed = pedal_pico_sideband_conversion_2 >> 7; // Make 5-bit Value (0-31)
    pedal_pico_sideband_osc_start_threshold = (pedal_pico_sideband_conversion_3 >> 7) * PEDAL_PICO_SIDEBAND_OSC_START_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    pedal_pico_sideband_osc_start_count = 0;
    //pedal_pico_sideband_debug_time = 0;
}

void pedal_pico_sideband_on_pwm_irq_wrap() {
    pwm_clear_irq(pedal_pico_sideband->pwm_1_slice);
    //uint32 from_time = time_us_32();
    uint16 conversion_1 = util_pedal_pico_on_adc_conversion_1;
    uint16 conversion_2 = util_pedal_pico_on_adc_conversion_2;
    uint16 conversion_3 = util_pedal_pico_on_adc_conversion_3;
    if (! util_pedal_pico_on_adc_is_outstanding) {
        util_pedal_pico_on_adc_is_outstanding = true;
        adc_select_input(0); // Ensure to Start from ADC0
        __dsb();
        __isb();
        adc_run(true); // Stable Starting Point after PWM IRQ
    }
    util_pedal_pico_renew_adc_middle_moving_average(conversion_1);
    pedal_pico_sideband_process(conversion_1, conversion_2, conversion_3, util_pedal_pico_sw_mode);
    /* Output */
    pwm_set_chan_level(pedal_pico_sideband->pwm_1_slice, pedal_pico_sideband->pwm_1_channel, (uint16)pedal_pico_sideband->output_1);
    pwm_set_chan_level(pedal_pico_sideband->pwm_2_slice, pedal_pico_sideband->pwm_2_channel, (uint16)pedal_pico_sideband->output_1_inverted);
    //pedal_pico_sideband_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_pico_sideband_debug_time); // To send a made pointer, sync flag, etc.
    __dsb();
}

void pedal_pico_sideband_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    pedal_pico_sideband_conversion_1 = conversion_1;
    if (abs(conversion_2 - pedal_pico_sideband_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_sideband_conversion_2 = conversion_2;
        pedal_pico_sideband_osc_speed = pedal_pico_sideband_conversion_2 >> 7; // Make 5-bit Value (0-31)
    }
    if (abs(conversion_3 - pedal_pico_sideband_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_sideband_conversion_3 = conversion_3;
        pedal_pico_sideband_osc_start_threshold = (pedal_pico_sideband_conversion_3 >> 7) * PEDAL_PICO_SIDEBAND_OSC_START_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    }
    int32 normalized_1 = (int32)pedal_pico_sideband_conversion_1 - (int32)util_pedal_pico_adc_middle_moving_average;
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
    if (sw_mode == 1) {
        /**
         * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
         * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
         * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
         */
        normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_sideband_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_SIDEBAND_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    } else if (sw_mode == 2) {
        normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_sideband_table_pdf_3[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_SIDEBAND_PWM_PEAK))]) >> 32);
    } else {
        normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_sideband_table_pdf_2[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_SIDEBAND_PWM_PEAK))]) >> 32);
    }
    normalized_1 = util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_SIDEBAND_CUTOFF_FIXED_1);
    int32 fixed_point_value_sine_1 = pedal_pico_sideband_table_sine_1[pedal_pico_sideband_osc_sine_1_index / PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER];
    int32 fixed_point_value_sine_2 = pedal_pico_sideband_table_sine_1[pedal_pico_sideband_osc_sine_2_index / PEDAL_PICO_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER] >> 1; // Divide By 2
    pedal_pico_sideband_osc_sine_1_index += pedal_pico_sideband_osc_speed;
    pedal_pico_sideband_osc_sine_2_index += pedal_pico_sideband_osc_speed;
    if (pedal_pico_sideband_osc_sine_1_index >= PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MAX * PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER) pedal_pico_sideband_osc_sine_1_index -= PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MAX * PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER;
    if (pedal_pico_sideband_osc_sine_2_index >= PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MAX * PEDAL_PICO_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER) pedal_pico_sideband_osc_sine_2_index -= PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MAX * PEDAL_PICO_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER;
    int32 osc_value = (int32)(int64)((((int64)pedal_pico_sideband_osc_amplitude << 16) * ((int64)fixed_point_value_sine_1 + (int64)fixed_point_value_sine_2)) >> 16); // Remain Decimal Part
    osc_value = (int32)(int64)(((int64)osc_value * ((int64)abs(normalized_1) << 3)) >> 32); // Absolute normalized_1 to Multiply Frequency
    osc_value *= PEDAL_PICO_SIDEBAND_GAIN;
    /* Output */
    pedal_pico_sideband->output_1 = util_pedal_pico_cutoff_biased(osc_value + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_PICO_SIDEBAND_PWM_OFFSET + PEDAL_PICO_SIDEBAND_PWM_PEAK, PEDAL_PICO_SIDEBAND_PWM_OFFSET - PEDAL_PICO_SIDEBAND_PWM_PEAK);
    pedal_pico_sideband->output_1_inverted = util_pedal_pico_cutoff_biased(-osc_value + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_PICO_SIDEBAND_PWM_OFFSET + PEDAL_PICO_SIDEBAND_PWM_PEAK, PEDAL_PICO_SIDEBAND_PWM_OFFSET - PEDAL_PICO_SIDEBAND_PWM_PEAK);
}

void pedal_pico_sideband_free() { // Free Except Object, pedal_pico_sideband
    __dsb();
}
