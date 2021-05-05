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

#include "pedal_pico/pedal_pico_distortion.h"

void pedal_pico_distortion_start() {
    if (! pedal_pico_distortion) panic("pedal_pico_distortion is not initialized.");
    /* PWM Settings */
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_pico_distortion_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0xF0);
    pwm_set_chan_level(pedal_pico_distortion->pwm_1_slice, pedal_pico_distortion->pwm_1_channel, PEDAL_PICO_DISTORTION_PWM_OFFSET);
    pwm_set_chan_level(pedal_pico_distortion->pwm_2_slice, pedal_pico_distortion->pwm_2_channel, PEDAL_PICO_DISTORTION_PWM_OFFSET);
    /* Unique Settings */
    pedal_pico_distortion_set();
    /* Start */
    util_pedal_pico_start((util_pedal_pico*)pedal_pico_distortion);
    util_pedal_pico_sw_loop(PEDAL_PICO_DISTORTION_SW_1_GPIO, PEDAL_PICO_DISTORTION_SW_2_GPIO);
}

void pedal_pico_distortion_set() {
    if (! pedal_pico_distortion) panic("pedal_pico_distortion is not initialized.");
    pedal_pico_distortion_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_distortion_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_distortion_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_distortion_loss = 32 - (pedal_pico_distortion_conversion_2 >> 7); // Make 5-bit Value (1-32)
}

void pedal_pico_distortion_on_pwm_irq_wrap() {
    pwm_clear_irq(pedal_pico_distortion->pwm_1_slice);
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
    pedal_pico_distortion_process(conversion_1, conversion_2, conversion_3, util_pedal_pico_sw_mode);
    /* Output */
    pwm_set_chan_level(pedal_pico_distortion->pwm_1_slice, pedal_pico_distortion->pwm_1_channel, (uint16)pedal_pico_distortion->output_1);
    pwm_set_chan_level(pedal_pico_distortion->pwm_2_slice, pedal_pico_distortion->pwm_2_channel, (uint16)pedal_pico_distortion->output_1_inverted);
    //pedal_pico_distortion_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_pico_distortion_debug_time); // To send a made pointer, sync flag, etc.
    __dsb();
}

void pedal_pico_distortion_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    pedal_pico_distortion_conversion_1 = conversion_1;
    if (abs(conversion_2 - pedal_pico_distortion_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_distortion_conversion_2 = conversion_2;
        pedal_pico_distortion_loss = 32 - (pedal_pico_distortion_conversion_2 >> 7); // Make 5-bit Value (1-32)
    }
    if (abs(conversion_3 - pedal_pico_distortion_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_distortion_conversion_3 = conversion_3;
    }
    int32 normalized_1 = (int32)pedal_pico_distortion_conversion_1 - (int32)util_pedal_pico_adc_middle_moving_average;
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_distortion_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_DISTORTION_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    normalized_1 = util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_DISTORTION_CUTOFF_FIXED_1);
    if (sw_mode == 1) {
        if (normalized_1 > 0) {
            normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_distortion_table_log_2[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_DISTORTION_PWM_PEAK))]) >> 32);
        } else {
            normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_distortion_table_power_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_DISTORTION_PWM_PEAK))]) >> 32);
        }
    } else if (sw_mode == 2) {
        if (normalized_1 > 0) {
            normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_distortion_table_log_2[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_DISTORTION_PWM_PEAK))]) >> 32);
        } else {
            normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_distortion_table_log_2[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_DISTORTION_PWM_PEAK))]) >> 32);
        }
    } else {
        if (normalized_1 > 0) {
            normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_distortion_table_log_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_DISTORTION_PWM_PEAK))]) >> 32);
        } else {
            normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_distortion_table_log_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_DISTORTION_PWM_PEAK))]) >> 32);
        }
    }
    normalized_1 /= pedal_pico_distortion_loss;
    normalized_1 *= PEDAL_PICO_DISTORTION_GAIN;
    /* Output */
    pedal_pico_distortion->output_1 = util_pedal_pico_cutoff_biased(normalized_1 + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_PICO_DISTORTION_PWM_OFFSET + PEDAL_PICO_DISTORTION_PWM_PEAK, PEDAL_PICO_DISTORTION_PWM_OFFSET - PEDAL_PICO_DISTORTION_PWM_PEAK);
    pedal_pico_distortion->output_1_inverted = util_pedal_pico_cutoff_biased(-normalized_1 + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_PICO_DISTORTION_PWM_OFFSET + PEDAL_PICO_DISTORTION_PWM_PEAK, PEDAL_PICO_DISTORTION_PWM_OFFSET - PEDAL_PICO_DISTORTION_PWM_PEAK);
}

void pedal_pico_distortion_free() { // Free Except Object, pedal_pico_distortion
    __dsb();
}
