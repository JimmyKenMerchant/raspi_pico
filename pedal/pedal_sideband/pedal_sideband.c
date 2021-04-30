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

// Standards
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// Dependancies
#include "pico/stdlib.h"
#include "pico/divider.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
// raspi_pico/include
#include "macros_pico.h"
#include "util_pedal_pico.h"
// Private header
#include "pedal_sideband.h"

#define PEDAL_SIDEBAND_TRANSIENT_RESPONSE 100000 // 100000 Micro Seconds
#define PEDAL_SIDEBAND_CORE_1_STACK_SIZE 1024 * 4 // 1024 Words, 4096 Bytes
#define PEDAL_SIDEBAND_LED_GPIO 25
#define PEDAL_SIDEBAND_SW_1_GPIO 14
#define PEDAL_SIDEBAND_SW_2_GPIO 15
#define PEDAL_SIDEBAND_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_SIDEBAND_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_SIDEBAND_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_SIDEBAND_PWM_PEAK 2047
#define PEDAL_SIDEBAND_GAIN 1
#define PEDAL_SIDEBAND_CUTOFF_FIXED_1 0xC0
#define PEDAL_SIDEBAND_OSC_SINE_1_TIME_MAX 9375
#define PEDAL_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER 6
#define PEDAL_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER 4
#define PEDAL_SIDEBAND_OSC_AMPLITUDE_PEAK 4095
#define PEDAL_SIDEBAND_OSC_START_THRESHOLD_MULTIPLIER 1 // From -66.22dB (Loss 2047) to -36.39dB (Loss 66) in ADC_VREF (Typically 3.3V)
#define PEDAL_SIDEBAND_OSC_START_COUNT_MAX 2000 // 28125 Divided by 2000 = Approx. 14Hz

volatile util_pedal_pico* pedal_sideband;
volatile uint16 pedal_sideband_conversion_1;
volatile uint16 pedal_sideband_conversion_2;
volatile uint16 pedal_sideband_conversion_3;
volatile uint32 pedal_sideband_osc_sine_1_index;
volatile uint32 pedal_sideband_osc_sine_2_index;
volatile uint16 pedal_sideband_osc_amplitude;
volatile uint16 pedal_sideband_osc_speed;
volatile char8 pedal_sideband_osc_start_threshold;
volatile uint16 pedal_sideband_osc_start_count;
volatile uint32 pedal_sideband_debug_time;

void pedal_sideband_core_1();
void pedal_sideband_on_pwm_irq_wrap();
void pedal_sideband_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3);
void pedal_sideband_free();

int main(void) {
    //stdio_init_all();
    util_pedal_pico_set_sys_clock_115200khz();
    //stdio_init_all(); // Re-init for UART Baud Rate
    sleep_us(PEDAL_SIDEBAND_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(PEDAL_SIDEBAND_LED_GPIO);
    gpio_set_dir(PEDAL_SIDEBAND_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_SIDEBAND_LED_GPIO, true);
    uint32* stack_pointer = (int32*)malloc(PEDAL_SIDEBAND_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(pedal_sideband_core_1, stack_pointer, PEDAL_SIDEBAND_CORE_1_STACK_SIZE);
    //pedal_sideband_debug_time = 0;
    //uint32 from_time = time_us_32();
    //printf("@main 1 - Let's Start!\n");
    //pedal_sideband_debug_time = time_us_32() - from_time;
    //printf("@main 2 - pedal_sideband_debug_time %d\n", pedal_sideband_debug_time);
    while (true) {
        //printf("@main 3 - pedal_sideband_conversion_1 %0x\n", pedal_sideband_conversion_1);
        //printf("@main 4 - pedal_sideband_conversion_2 %0x\n", pedal_sideband_conversion_2);
        //printf("@main 5 - pedal_sideband_conversion_3 %0x\n", pedal_sideband_conversion_3);
        //printf("@main 6 - multicore_fifo_pop_blocking() %d\n", multicore_fifo_pop_blocking());
        //printf("@main 7 - pedal_sideband_debug_time %d\n", pedal_sideband_debug_time);
        //sleep_ms(500);
        //tight_loop_contents();
        __wfi();
    }
    return 0;
}

void pedal_sideband_core_1() {
    /* PWM Settings */
    pedal_sideband = util_pedal_pico_init(PEDAL_SIDEBAND_PWM_1_GPIO, PEDAL_SIDEBAND_PWM_2_GPIO);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_sideband_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0xF0);
    pwm_set_chan_level(pedal_sideband->pwm_1_slice, pedal_sideband->pwm_1_channel, PEDAL_SIDEBAND_PWM_OFFSET);
    pwm_set_chan_level(pedal_sideband->pwm_2_slice, pedal_sideband->pwm_2_channel, PEDAL_SIDEBAND_PWM_OFFSET);
    /* ADC Settings */
    util_pedal_pico_init_adc();
    /* Unique Settings */
    pedal_sideband_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_sideband_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_sideband_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_sideband_osc_sine_1_index = 0;
    pedal_sideband_osc_sine_2_index = 0;
    pedal_sideband_osc_amplitude = PEDAL_SIDEBAND_OSC_AMPLITUDE_PEAK;
    pedal_sideband_osc_speed = pedal_sideband_conversion_2 >> 7; // Make 5-bit Value (0-31)
    pedal_sideband_osc_start_threshold = (pedal_sideband_conversion_3 >> 7) * PEDAL_SIDEBAND_OSC_START_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    pedal_sideband_osc_start_count = 0;
    /* Start */
    util_pedal_pico_start((util_pedal_pico*)pedal_sideband);
    util_pedal_pico_sw_loop(PEDAL_SIDEBAND_SW_1_GPIO, PEDAL_SIDEBAND_SW_2_GPIO);
}

void pedal_sideband_on_pwm_irq_wrap() {
    pwm_clear_irq(pedal_sideband->pwm_1_slice);
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
    pedal_sideband_process(conversion_1, conversion_2, conversion_3);
    /* Output */
    pwm_set_chan_level(pedal_sideband->pwm_1_slice, pedal_sideband->pwm_1_channel, (uint16)pedal_sideband->output_1);
    pwm_set_chan_level(pedal_sideband->pwm_2_slice, pedal_sideband->pwm_2_channel, (uint16)pedal_sideband->output_1_inverted);
    //pedal_sideband_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_sideband_debug_time); // To send a made pointer, sync flag, etc.
    __dsb();
}

void pedal_sideband_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3) {
    pedal_sideband_conversion_1 = conversion_1;
    if (abs(conversion_2 - pedal_sideband_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_sideband_conversion_2 = conversion_2;
        pedal_sideband_osc_speed = pedal_sideband_conversion_2 >> 7; // Make 5-bit Value (0-31)
    }
    if (abs(conversion_3 - pedal_sideband_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_sideband_conversion_3 = conversion_3;
        pedal_sideband_osc_start_threshold = (pedal_sideband_conversion_3 >> 7) * PEDAL_SIDEBAND_OSC_START_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    }
    int32 normalized_1 = (int32)pedal_sideband_conversion_1 - (int32)util_pedal_pico_adc_middle_moving_average;
    /**
     * pedal_sideband_osc_start_count:
     *
     * Over Positive Threshold       ## 1
     *-----------------------------------------------------------------------------------------------------------
     * Under Positive Threshold     # 0 # 2      ### Reset to 1
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                  # 0   # 3   # 5   # 2
     *-----------------------------------------------------------------------------------------------------------
     * 0                           # 0   # 4   # 4   # 3   # 5 ...Count Up to PEDAL_SIDEBAND_OSC_START_COUNT_MAX
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                         # 5 # 3      #### 4
     *-----------------------------------------------------------------------------------------------------------
     * Under Negative Threshold           # 6 # 2
     *-----------------------------------------------------------------------------------------------------------
     * Over Negative Threshold             ## Reset to 1
     */
    if (normalized_1 > pedal_sideband_osc_start_threshold || normalized_1 < -pedal_sideband_osc_start_threshold) {
        pedal_sideband_osc_start_count = 1;
    } else if (pedal_sideband_osc_start_count != 0 && (normalized_1 > (pedal_sideband_osc_start_threshold >> 1) || normalized_1 < -(pedal_sideband_osc_start_threshold >> 1))) {
        pedal_sideband_osc_start_count = 1;
    } else if (pedal_sideband_osc_start_count != 0) {
        pedal_sideband_osc_start_count++;
    }
    if (pedal_sideband_osc_start_count >= PEDAL_SIDEBAND_OSC_START_COUNT_MAX) pedal_sideband_osc_start_count = 0;
    if (pedal_sideband_osc_start_count == 0) {
        pedal_sideband_osc_sine_1_index = 0;
        pedal_sideband_osc_sine_2_index = 0;
    }
    if (util_pedal_pico_sw_mode == 1) {
        /**
         * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
         * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
         * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
         */
        normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_sideband_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_SIDEBAND_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    } else if (util_pedal_pico_sw_mode == 2) {
        normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_sideband_table_pdf_3[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_SIDEBAND_PWM_PEAK))]) >> 32);
    } else {
        normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_sideband_table_pdf_2[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_SIDEBAND_PWM_PEAK))]) >> 32);
    }
    normalized_1 = util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_SIDEBAND_CUTOFF_FIXED_1);
    int32 fixed_point_value_sine_1 = pedal_sideband_table_sine_1[pedal_sideband_osc_sine_1_index / PEDAL_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER];
    int32 fixed_point_value_sine_2 = pedal_sideband_table_sine_1[pedal_sideband_osc_sine_2_index / PEDAL_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER] >> 1; // Divide By 2
    pedal_sideband_osc_sine_1_index += pedal_sideband_osc_speed;
    pedal_sideband_osc_sine_2_index += pedal_sideband_osc_speed;
    if (pedal_sideband_osc_sine_1_index >= PEDAL_SIDEBAND_OSC_SINE_1_TIME_MAX * PEDAL_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER) pedal_sideband_osc_sine_1_index -= PEDAL_SIDEBAND_OSC_SINE_1_TIME_MAX * PEDAL_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER;
    if (pedal_sideband_osc_sine_2_index >= PEDAL_SIDEBAND_OSC_SINE_1_TIME_MAX * PEDAL_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER) pedal_sideband_osc_sine_2_index -= PEDAL_SIDEBAND_OSC_SINE_1_TIME_MAX * PEDAL_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER;
    int32 osc_value = (int32)(int64)((((int64)pedal_sideband_osc_amplitude << 16) * ((int64)fixed_point_value_sine_1 + (int64)fixed_point_value_sine_2)) >> 16); // Remain Decimal Part
    osc_value = (int32)(int64)(((int64)osc_value * ((int64)abs(normalized_1) << 3)) >> 32); // Absolute normalized_1 to Multiply Frequency
    osc_value *= PEDAL_SIDEBAND_GAIN;
    /* Output */
    pedal_sideband->output_1 = util_pedal_pico_cutoff_biased(osc_value + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_SIDEBAND_PWM_OFFSET + PEDAL_SIDEBAND_PWM_PEAK, PEDAL_SIDEBAND_PWM_OFFSET - PEDAL_SIDEBAND_PWM_PEAK);
    pedal_sideband->output_1_inverted = util_pedal_pico_cutoff_biased(-osc_value + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_SIDEBAND_PWM_OFFSET + PEDAL_SIDEBAND_PWM_PEAK, PEDAL_SIDEBAND_PWM_OFFSET - PEDAL_SIDEBAND_PWM_PEAK);
}

void pedal_sideband_free() { // Free Except Object, pedal_sideband
    util_pedal_pico_stop((util_pedal_pico*)pedal_sideband);
    irq_remove_handler(PWM_IRQ_WRAP, pedal_sideband_on_pwm_irq_wrap);
    __dsb();
}
