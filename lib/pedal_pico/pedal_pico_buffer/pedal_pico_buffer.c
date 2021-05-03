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

#include "pedal_pico/pedal_pico_buffer.h"

void pedal_pico_buffer_start() {
    if (! pedal_pico_buffer) panic("pedal_pico_buffer is not initialized.");
    /* PWM Settings */
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_pico_buffer_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0xF0);
    pwm_set_chan_level(pedal_pico_buffer->pwm_1_slice, pedal_pico_buffer->pwm_1_channel, PEDAL_PICO_BUFFER_PWM_OFFSET);
    pwm_set_chan_level(pedal_pico_buffer->pwm_2_slice, pedal_pico_buffer->pwm_2_channel, PEDAL_PICO_BUFFER_PWM_OFFSET);
    /* Unique Settings */
    pedal_pico_buffer_set();
    /* Start */
    util_pedal_pico_start((util_pedal_pico*)pedal_pico_buffer);
    util_pedal_pico_sw_loop(PEDAL_PICO_BUFFER_SW_1_GPIO, PEDAL_PICO_BUFFER_SW_2_GPIO);
}

void pedal_pico_buffer_set() {
    if (! pedal_pico_buffer) panic("pedal_pico_buffer is not initialized.");
    pedal_pico_buffer_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_buffer_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_buffer_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_pico_buffer_delay_array = (int16*)calloc(PEDAL_PICO_BUFFER_DELAY_TIME_MAX, sizeof(int16));
    pedal_pico_buffer_delay_amplitude = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_1;
    pedal_pico_buffer_delay_amplitude_interpolation = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_1;
    pedal_pico_buffer_delay_amplitude_interpolation_accum = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_1;
    uint16 delay_time = (pedal_pico_buffer_conversion_2 >> 7) << PEDAL_PICO_BUFFER_DELAY_TIME_SHIFT; // Make 5-bit Value (0-31) and Shift
    pedal_pico_buffer_delay_time = delay_time;
    pedal_pico_buffer_delay_time_interpolation = delay_time;
    pedal_pico_buffer_delay_index = 0;
    pedal_pico_buffer_noise_gate_threshold = (pedal_pico_buffer_conversion_3 >> 7) * PEDAL_PICO_BUFFER_NOISE_GATE_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    pedal_pico_buffer_noise_gate_count = 0;
}

void pedal_pico_buffer_on_pwm_irq_wrap() {
    pwm_clear_irq(pedal_pico_buffer->pwm_1_slice);
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
    pedal_pico_buffer_process(conversion_1, conversion_2, conversion_3, util_pedal_pico_sw_mode);
    /* Output */
    pwm_set_chan_level(pedal_pico_buffer->pwm_1_slice, pedal_pico_buffer->pwm_1_channel, (uint16)pedal_pico_buffer->output_1);
    pwm_set_chan_level(pedal_pico_buffer->pwm_2_slice, pedal_pico_buffer->pwm_2_channel, (uint16)pedal_pico_buffer->output_1_inverted);
    //pedal_pico_buffer_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_pico_buffer_debug_time); // To send a made pointer, sync flag, etc.
    __dsb();
}

void pedal_pico_buffer_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    pedal_pico_buffer_conversion_1 = conversion_1;
    if (abs(conversion_2 - pedal_pico_buffer_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_buffer_conversion_2 = conversion_2;
        pedal_pico_buffer_delay_time = (pedal_pico_buffer_conversion_2 >> 7) << PEDAL_PICO_BUFFER_DELAY_TIME_SHIFT; // Make 5-bit Value (0-31) and Shift
    }
    if (abs(conversion_3 - pedal_pico_buffer_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_pico_buffer_conversion_3 = conversion_3;
        pedal_pico_buffer_noise_gate_threshold = (pedal_pico_buffer_conversion_3 >> 7) * PEDAL_PICO_BUFFER_NOISE_GATE_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    }
    pedal_pico_buffer_delay_time_interpolation = util_pedal_pico_interpolate(pedal_pico_buffer_delay_time_interpolation, pedal_pico_buffer_delay_time, PEDAL_PICO_BUFFER_DELAY_TIME_INTERPOLATION_ACCUM);
    int32 normalized_1 = (int32)pedal_pico_buffer_conversion_1 - (int32)util_pedal_pico_adc_middle_moving_average;
    /**
     * pedal_pico_buffer_noise_gate_count:
     *
     * Over Positive Threshold       ## 1
     *-----------------------------------------------------------------------------------------------------------
     * Under Positive Threshold     # 0 # 2      ### Reset to 1
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                  # 0   # 3   # 5   # 2
     *-----------------------------------------------------------------------------------------------------------
     * 0                           # 0   # 4   # 4   # 3   # 5 ...Count Up to PEDAL_PICO_BUFFER_NOISE_GATE_COUNT_MAX
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                         # 5 # 3      #### 4
     *-----------------------------------------------------------------------------------------------------------
     * Under Negative Threshold           # 6 # 2
     *-----------------------------------------------------------------------------------------------------------
     * Over Negative Threshold             ## Reset to 1
     */
    if (normalized_1 > pedal_pico_buffer_noise_gate_threshold || normalized_1 < -pedal_pico_buffer_noise_gate_threshold) {
        pedal_pico_buffer_noise_gate_count = 1;
        pedal_pico_buffer_delay_amplitude = 0x00000000;
    } else if (pedal_pico_buffer_noise_gate_count != 0 && (normalized_1 > (pedal_pico_buffer_noise_gate_threshold >> 1) || normalized_1 < -(pedal_pico_buffer_noise_gate_threshold >> 1))) {
        pedal_pico_buffer_noise_gate_count = 1;
    } else if (pedal_pico_buffer_noise_gate_count != 0) {
        pedal_pico_buffer_noise_gate_count++;
    }
    if (pedal_pico_buffer_noise_gate_count >= PEDAL_PICO_BUFFER_NOISE_GATE_COUNT_MAX) {
        pedal_pico_buffer_noise_gate_count = 0;
        if (sw_mode == 1) {
            pedal_pico_buffer_delay_amplitude = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_1;
            pedal_pico_buffer_delay_amplitude_interpolation_accum = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_1;
        } else if (sw_mode == 2) {
            pedal_pico_buffer_delay_amplitude = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_3;
            pedal_pico_buffer_delay_amplitude_interpolation_accum = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_3;
        } else {
            pedal_pico_buffer_delay_amplitude = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_2;
            pedal_pico_buffer_delay_amplitude_interpolation_accum = PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_2;
        }
    }
    pedal_pico_buffer_delay_amplitude_interpolation = util_pedal_pico_interpolate(pedal_pico_buffer_delay_amplitude_interpolation, pedal_pico_buffer_delay_amplitude, pedal_pico_buffer_delay_amplitude_interpolation_accum);
    if (pedal_pico_buffer_noise_gate_count == 0) {
        normalized_1 >>= PEDAL_PICO_BUFFER_NOISE_GATE_REDUCE_SHIFT;
    }
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_buffer_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PICO_BUFFER_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    /* Make Sustain */
    int32 delay_1 = (int32)pedal_pico_buffer_delay_array[((pedal_pico_buffer_delay_index + PEDAL_PICO_BUFFER_DELAY_TIME_MAX) - pedal_pico_buffer_delay_time_interpolation) % PEDAL_PICO_BUFFER_DELAY_TIME_MAX];
    if (pedal_pico_buffer_delay_time_interpolation == 0) delay_1 = 0; // No Reverb, Otherwise Latest
    int32 pedal_pico_buffer_normalized_1_amplitude = 0x00010000 - pedal_pico_buffer_delay_amplitude_interpolation;
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_pico_buffer_normalized_1_amplitude) >> 32);
    delay_1 = (int32)(int64)((((int64)delay_1 << 16) * (int64)pedal_pico_buffer_delay_amplitude_interpolation) >> 32);
    int32 mixed_1 = normalized_1 + delay_1;
    pedal_pico_buffer_delay_array[pedal_pico_buffer_delay_index] = (int16)mixed_1;
    pedal_pico_buffer_delay_index++;
    if (pedal_pico_buffer_delay_index >= PEDAL_PICO_BUFFER_DELAY_TIME_MAX) pedal_pico_buffer_delay_index -= PEDAL_PICO_BUFFER_DELAY_TIME_MAX;
    mixed_1 *= PEDAL_PICO_BUFFER_GAIN;
    pedal_pico_buffer->output_1 = util_pedal_pico_cutoff_biased(mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_PICO_BUFFER_PWM_OFFSET + PEDAL_PICO_BUFFER_PWM_PEAK, PEDAL_PICO_BUFFER_PWM_OFFSET - PEDAL_PICO_BUFFER_PWM_PEAK);
    pedal_pico_buffer->output_1_inverted = util_pedal_pico_cutoff_biased(-mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_PICO_BUFFER_PWM_OFFSET + PEDAL_PICO_BUFFER_PWM_PEAK, PEDAL_PICO_BUFFER_PWM_OFFSET - PEDAL_PICO_BUFFER_PWM_PEAK);
}

void pedal_pico_buffer_free() { // Free Except Object, pedal_pico_buffer
    util_pedal_pico_stop((util_pedal_pico*)pedal_pico_buffer);
    irq_remove_handler(PWM_IRQ_WRAP, pedal_pico_buffer_on_pwm_irq_wrap);
    free((void*)pedal_pico_buffer_delay_array);
    __dsb();
}
