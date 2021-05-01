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
#include "util_pedal_pico_ex.h"
// Private header
//#include "pedal_chorus.h"

#define PEDAL_CHORUS_TRANSIENT_RESPONSE 100000 // 100000 Micro Seconds
#define PEDAL_CHORUS_CORE_1_STACK_SIZE 1024 * 4 // 1024 Words, 4096 Bytes
#define PEDAL_CHORUS_LED_GPIO 25
#define PEDAL_CHORUS_SW_1_GPIO 14
#define PEDAL_CHORUS_SW_2_GPIO 15
#define PEDAL_CHORUS_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_CHORUS_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_CHORUS_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_CHORUS_PWM_PEAK 2047
#define PEDAL_CHORUS_GAIN 1
#define PEDAL_CHORUS_DELAY_AMPLITUDE_FIXED_1 (int32)(0x00010000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_CHORUS_DELAY_TIME_MAX 1527
#define PEDAL_CHORUS_DELAY_TIME_FIXED_1 PEDAL_CHORUS_DELAY_TIME_MAX - 1 // 1526 Divided by 28125 (0.054 Seconds)
#define PEDAL_CHORUS_OSC_SINE_1_TIME_MAX 9375
#define PEDAL_CHORUS_OSC_SINE_1_TIME_MULTIPLIER 6
#define PEDAL_CHORUS_LR_DISTANCE_TIME_MAX 993
#define PEDAL_CHORUS_LR_DISTANCE_TIME_SHIFT 5 // Multiply By 32 (0-992), 992 Divided by 28125 (0.0353 Seconds = 12.01 Meters)
#define PEDAL_CHORUS_LR_DISTANCE_TIME_INTERPOLATION_ACCUM 1 // Value to Accumulate

volatile util_pedal_pico* pedal_chorus;
volatile uint16 pedal_chorus_conversion_1;
volatile uint16 pedal_chorus_conversion_2;
volatile uint16 pedal_chorus_conversion_3;
volatile uint32 pedal_chorus_osc_sine_1_index;
volatile uint16 pedal_chorus_osc_speed;
volatile int16* pedal_chorus_delay_array;
volatile int32 pedal_chorus_delay_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
volatile uint16 pedal_chorus_delay_time;
volatile uint16 pedal_chorus_delay_index;
volatile int16* pedal_chorus_lr_distance_array;
volatile uint16 pedal_chorus_lr_distance_time;
volatile uint16 pedal_chorus_lr_distance_time_interpolation;
volatile uint16 pedal_chorus_lr_distance_index;
volatile uint32 pedal_chorus_debug_time;

void pedal_chorus_core_1();
void pedal_chorus_set();
void pedal_chorus_on_pwm_irq_wrap();
void pedal_chorus_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3);
void pedal_chorus_free();

int main(void) {
    //stdio_init_all();
    util_pedal_pico_set_sys_clock_115200khz();
    //stdio_init_all(); // Re-init for UART Baud Rate
    sleep_us(PEDAL_CHORUS_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(PEDAL_CHORUS_LED_GPIO);
    gpio_set_dir(PEDAL_CHORUS_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_CHORUS_LED_GPIO, true);
    uint32* stack_pointer = (int32*)malloc(PEDAL_CHORUS_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(pedal_chorus_core_1, stack_pointer, PEDAL_CHORUS_CORE_1_STACK_SIZE);
    //pedal_chorus_debug_time = 0;
    //uint32 from_time = time_us_32();
    //printf("@main 1 - Let's Start!\n");
    //pedal_chorus_debug_time = time_us_32() - from_time;
    //printf("@main 2 - pedal_chorus_debug_time %d\n", pedal_chorus_debug_time);
    while (true) {
        //printf("@main 3 - pedal_chorus_conversion_1 %0x\n", pedal_chorus_conversion_1);
        //printf("@main 4 - pedal_chorus_conversion_2 %0x\n", pedal_chorus_conversion_2);
        //printf("@main 5 - pedal_chorus_conversion_3 %0x\n", pedal_chorus_conversion_3);
        //printf("@main 6 - multicore_fifo_pop_blocking() %d\n", multicore_fifo_pop_blocking());
        //printf("@main 7 - pedal_chorus_debug_time %d\n", pedal_chorus_debug_time);
        //sleep_ms(500);
        //tight_loop_contents();
        __wfi();
    }
    return 0;
}

void pedal_chorus_core_1() {
    /* PWM Settings */
    pedal_chorus = util_pedal_pico_init(PEDAL_CHORUS_PWM_1_GPIO, PEDAL_CHORUS_PWM_2_GPIO);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_chorus_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0xF0);
    pwm_set_chan_level(pedal_chorus->pwm_1_slice, pedal_chorus->pwm_1_channel, PEDAL_CHORUS_PWM_OFFSET);
    pwm_set_chan_level(pedal_chorus->pwm_2_slice, pedal_chorus->pwm_2_channel, PEDAL_CHORUS_PWM_OFFSET);
    /* ADC Settings */
    util_pedal_pico_init_adc();
    /* Unique Settings */
    pedal_chorus_set();
    /* Start */
    util_pedal_pico_start((util_pedal_pico*)pedal_chorus);
    util_pedal_pico_sw_loop(PEDAL_CHORUS_SW_1_GPIO, PEDAL_CHORUS_SW_2_GPIO);
}

void pedal_chorus_set() {
    pedal_chorus_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_chorus_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_chorus_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_chorus_delay_array = (int16*)calloc(PEDAL_CHORUS_DELAY_TIME_MAX, sizeof(int16));
    pedal_chorus_delay_amplitude = PEDAL_CHORUS_DELAY_AMPLITUDE_FIXED_1;
    pedal_chorus_delay_time = PEDAL_CHORUS_DELAY_TIME_FIXED_1;
    pedal_chorus_delay_index = 0;
    pedal_chorus_osc_speed = pedal_chorus_conversion_2 >> 7; // Make 5-bit Value (0-31)
    pedal_chorus_osc_sine_1_index = 0;
    pedal_chorus_lr_distance_array =  (int16*)calloc(PEDAL_CHORUS_LR_DISTANCE_TIME_MAX, sizeof(int16));
    uint16 lr_distance_time = (pedal_chorus_conversion_3 >> 7) << PEDAL_CHORUS_LR_DISTANCE_TIME_SHIFT; // Make 5-bit Value (0-31) and Shift
    pedal_chorus_lr_distance_time = lr_distance_time;
    pedal_chorus_lr_distance_time_interpolation = lr_distance_time;
    pedal_chorus_lr_distance_index = 0;
}

void pedal_chorus_on_pwm_irq_wrap() {
    pwm_clear_irq(pedal_chorus->pwm_1_slice);
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
    pedal_chorus_process(conversion_1, conversion_2, conversion_3);
    /* Output */
    pwm_set_chan_level(pedal_chorus->pwm_1_slice, pedal_chorus->pwm_1_channel, (uint16)pedal_chorus->output_1);
    pwm_set_chan_level(pedal_chorus->pwm_2_slice, pedal_chorus->pwm_2_channel, (uint16)pedal_chorus->output_1_inverted);
    //pedal_chorus_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_chorus_debug_time); // To send a made pointer, sync flag, etc.
    __dsb();
}

void pedal_chorus_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3) {
    pedal_chorus_conversion_1 = conversion_1;
    if (abs(conversion_2 - pedal_chorus_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_chorus_conversion_2 = conversion_2;
        pedal_chorus_osc_speed = pedal_chorus_conversion_2 >> 7; // Make 5-bit Value (0-31)
    }
    if (abs(conversion_3 - pedal_chorus_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_chorus_conversion_3 = conversion_3;
        pedal_chorus_lr_distance_time = (pedal_chorus_conversion_3 >> 7) << PEDAL_CHORUS_LR_DISTANCE_TIME_SHIFT; // Make 5-bit Value (0-31) and Shift
    }
    pedal_chorus_lr_distance_time_interpolation = util_pedal_pico_interpolate(pedal_chorus_lr_distance_time_interpolation, pedal_chorus_lr_distance_time, PEDAL_CHORUS_LR_DISTANCE_TIME_INTERPOLATION_ACCUM);
    int32 normalized_1 = (int32)pedal_chorus_conversion_1 - (int32)util_pedal_pico_adc_middle_moving_average;
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)util_pedal_pico_ex_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_CHORUS_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    /* Push and Pop Delay */
    pedal_chorus_delay_array[pedal_chorus_delay_index] = (int16)normalized_1; // Push Current Value in Advance for 0
    int32 delay_1 = (int32)pedal_chorus_delay_array[((pedal_chorus_delay_index + PEDAL_CHORUS_DELAY_TIME_MAX) - pedal_chorus_delay_time) % PEDAL_CHORUS_DELAY_TIME_MAX];
    pedal_chorus_delay_index++;
    if (pedal_chorus_delay_index >= PEDAL_CHORUS_DELAY_TIME_MAX) pedal_chorus_delay_index -= PEDAL_CHORUS_DELAY_TIME_MAX;
    /* Get Oscillator */
    int32 fixed_point_value_sine_1 = util_pedal_pico_ex_table_sine_1[pedal_chorus_osc_sine_1_index / PEDAL_CHORUS_OSC_SINE_1_TIME_MULTIPLIER];
    pedal_chorus_osc_sine_1_index += pedal_chorus_osc_speed;
    if (pedal_chorus_osc_sine_1_index >= PEDAL_CHORUS_OSC_SINE_1_TIME_MAX * PEDAL_CHORUS_OSC_SINE_1_TIME_MULTIPLIER) pedal_chorus_osc_sine_1_index -= PEDAL_CHORUS_OSC_SINE_1_TIME_MAX * PEDAL_CHORUS_OSC_SINE_1_TIME_MULTIPLIER;
    delay_1 = (int32)(int64)((((int64)delay_1 << 16) * (int64)pedal_chorus_delay_amplitude) >> 32);
    int32 delay_1_l = (int32)(int64)((((int64)delay_1 << 16) * (int64)abs(fixed_point_value_sine_1)) >> 32);
    int32 delay_1_r = (int32)(int64)((((int64)delay_1 << 16) * (int64)(0x00010000 - abs(fixed_point_value_sine_1))) >> 32);
    /* Push and Pop Distance */
    pedal_chorus_lr_distance_array[pedal_chorus_lr_distance_index] = (int16)(int32)((normalized_1 + delay_1_r) >> 1); // Push Current Value in Advance for 0
    int32 lr_distance_1 = (int32)pedal_chorus_lr_distance_array[((pedal_chorus_lr_distance_index + PEDAL_CHORUS_LR_DISTANCE_TIME_MAX) - pedal_chorus_lr_distance_time_interpolation) % PEDAL_CHORUS_LR_DISTANCE_TIME_MAX];
    pedal_chorus_lr_distance_index++;
    if (pedal_chorus_lr_distance_index >= PEDAL_CHORUS_LR_DISTANCE_TIME_MAX) pedal_chorus_lr_distance_index -= PEDAL_CHORUS_LR_DISTANCE_TIME_MAX;
    /* Output */
    pedal_chorus->output_1 = util_pedal_pico_cutoff_biased(((normalized_1 + delay_1_l) >> 1) * PEDAL_CHORUS_GAIN + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_CHORUS_PWM_OFFSET + PEDAL_CHORUS_PWM_PEAK, PEDAL_CHORUS_PWM_OFFSET - PEDAL_CHORUS_PWM_PEAK);
    pedal_chorus->output_1_inverted = util_pedal_pico_cutoff_biased(-lr_distance_1 * PEDAL_CHORUS_GAIN + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_CHORUS_PWM_OFFSET + PEDAL_CHORUS_PWM_PEAK, PEDAL_CHORUS_PWM_OFFSET - PEDAL_CHORUS_PWM_PEAK);
}

void pedal_chorus_free() { // Free Except Object, pedal_chorus
    util_pedal_pico_stop((util_pedal_pico*)pedal_chorus);
    irq_remove_handler(PWM_IRQ_WRAP, pedal_chorus_on_pwm_irq_wrap);
    free((void*)pedal_chorus_delay_array);
    free((void*)pedal_chorus_lr_distance_array);
    __dsb();
}
