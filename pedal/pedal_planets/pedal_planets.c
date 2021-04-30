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
#include "hardware/resets.h"
// raspi_pico/include
#include "macros_pico.h"
#include "util_pedal_pico.h"
// Private header
#include "pedal_planets.h"

#define PEDAL_PLANETS_TRANSIENT_RESPONSE 100000 // 100000 Micro Seconds
#define PEDAL_PLANETS_CORE_1_STACK_SIZE 1024 * 4 // 1024 Words, 4096 Bytes
#define PEDAL_PLANETS_LED_GPIO 25
#define PEDAL_PLANETS_SW_1_GPIO 14
#define PEDAL_PLANETS_SW_2_GPIO 15
#define PEDAL_PLANETS_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_PLANETS_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_PLANETS_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_PLANETS_PWM_PEAK 2047
#define PEDAL_PLANETS_GAIN 1
#define PEDAL_PLANETS_COEFFICIENT_PEAK (int32)(0x00010000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PLANETS_COEFFICIENT_SHIFT 11 // Multiply by 4096 (0x00000800-0x00010000)
#define PEDAL_PLANETS_COEFFICIENT_INTERPOLATION_ACCUM 0x80 // Value to Accumulate
#define PEDAL_PLANETS_DELAY_TIME_MAX 2049 // Don't Use Delay Time = 0
#define PEDAL_PLANETS_DELAY_TIME_SHIFT 6 // Multiply by 64 (64-2048)
#define PEDAL_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_1 1 // Value to Accumulate, Small Value Makes Froggy
#define PEDAL_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_2 16 // Value to Accumulate

volatile util_pedal_pico* pedal_planets;
volatile uint16 pedal_planets_conversion_1;
volatile uint16 pedal_planets_conversion_2;
volatile uint16 pedal_planets_conversion_3;
volatile int32 pedal_planets_coefficient;
volatile int32 pedal_planets_coefficient_interpolation;
volatile int16* pedal_planets_delay_x;
volatile int16* pedal_planets_delay_y;
volatile uint16 pedal_planets_delay_time;
volatile uint16 pedal_planets_delay_time_interpolation;
volatile uint16 pedal_planets_delay_time_interpolation_accum;
volatile uint16 pedal_planets_delay_index;
volatile uint32 pedal_planets_debug_time;

void pedal_planets_core_1();
void pedal_planets_on_pwm_irq_wrap();
void pedal_planets_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3);
void pedal_planets_free();

int main(void) {
    //stdio_init_all();
    util_pedal_pico_set_sys_clock_115200khz();
    //stdio_init_all(); // Re-init for UART Baud Rate
    sleep_us(PEDAL_PLANETS_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(PEDAL_PLANETS_LED_GPIO);
    gpio_set_dir(PEDAL_PLANETS_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_PLANETS_LED_GPIO, true);
    uint32* stack_pointer = (int32*)malloc(PEDAL_PLANETS_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(pedal_planets_core_1, stack_pointer, PEDAL_PLANETS_CORE_1_STACK_SIZE);
    //pedal_planets_debug_time = 0;
    //uint32 from_time = time_us_32();
    //printf("@main 1 - Let's Start!\n");
    //pedal_planets_debug_time = time_us_32() - from_time;
    //printf("@main 2 - pedal_planets_debug_time %d\n", pedal_planets_debug_time);
    while (true) {
        //printf("@main 3 - pedal_planets_conversion_1 %0x\n", pedal_planets_conversion_1);
        //printf("@main 4 - pedal_planets_conversion_2 %0x\n", pedal_planets_conversion_2);
        //printf("@main 5 - pedal_planets_conversion_3 %0x\n", pedal_planets_conversion_3);
        //printf("@main 6 - multicore_fifo_pop_blocking() %d\n", multicore_fifo_pop_blocking());
        //printf("@main 7 - pedal_planets_debug_time %d\n", pedal_planets_debug_time);
        //sleep_ms(500);
        //tight_loop_contents();
        __wfi();
    }
    return 0;
}

void pedal_planets_core_1() {
    /* PWM Settings */
    pedal_planets = util_pedal_pico_init(PEDAL_PLANETS_PWM_1_GPIO, PEDAL_PLANETS_PWM_2_GPIO);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_planets_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0xF0);
    pwm_set_chan_level(pedal_planets->pwm_1_slice, pedal_planets->pwm_1_channel, PEDAL_PLANETS_PWM_OFFSET);
    pwm_set_chan_level(pedal_planets->pwm_2_slice, pedal_planets->pwm_2_channel, PEDAL_PLANETS_PWM_OFFSET);
    /* ADC Settings */
    util_pedal_pico_init_adc();
    /* Unique Settings */
    pedal_planets_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_planets_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_planets_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    int32 coefficient = ((pedal_planets_conversion_2 >> 7) + 1) << PEDAL_PLANETS_COEFFICIENT_SHIFT; // Make 5-bit Value (0-31) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    pedal_planets_coefficient = coefficient;
    pedal_planets_coefficient_interpolation = coefficient;
    pedal_planets_delay_x = (int16*)calloc(PEDAL_PLANETS_DELAY_TIME_MAX, sizeof(int16));
    pedal_planets_delay_y = (int16*)calloc(PEDAL_PLANETS_DELAY_TIME_MAX, sizeof(int16));
    uint16 delay_time = ((pedal_planets_conversion_3 >> 7) + 1) << PEDAL_PLANETS_DELAY_TIME_SHIFT; // Make 5-bit Value (0-31) and Shift
    pedal_planets_delay_time = delay_time;
    pedal_planets_delay_time_interpolation = delay_time;
    pedal_planets_delay_time_interpolation_accum = PEDAL_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_1;
    pedal_planets_delay_index = 0;
    /* Start */
    util_pedal_pico_start((util_pedal_pico*)pedal_planets);
    util_pedal_pico_sw_loop(PEDAL_PLANETS_SW_1_GPIO, PEDAL_PLANETS_SW_2_GPIO);
}

void pedal_planets_on_pwm_irq_wrap() {
    pwm_clear_irq(pedal_planets->pwm_1_slice);
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
    pedal_planets_process(conversion_1, conversion_2, conversion_3);
    /* Output */
    pwm_set_chan_level(pedal_planets->pwm_1_slice, pedal_planets->pwm_1_channel, (uint16)pedal_planets->output_1);
    pwm_set_chan_level(pedal_planets->pwm_2_slice, pedal_planets->pwm_2_channel, (uint16)pedal_planets->output_1_inverted);
    //pedal_planets_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_planets_debug_time); // To send a made pointer, sync flag, etc.
    __dsb();
}

void pedal_planets_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3) {
    pedal_planets_conversion_1 = conversion_1;
    if (abs(conversion_2 - pedal_planets_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_planets_conversion_2 = conversion_2;
        pedal_planets_coefficient = ((pedal_planets_conversion_2 >> 7) + 1) << PEDAL_PLANETS_COEFFICIENT_SHIFT; // Make 5-bit Value (0-31) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    }
    if (abs(conversion_3 - pedal_planets_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_planets_conversion_3 = conversion_3;
        pedal_planets_delay_time = ((pedal_planets_conversion_3 >> 7) + 1) << PEDAL_PLANETS_DELAY_TIME_SHIFT; // Make 5-bit Value (0-31) and Shift
    }
    pedal_planets_coefficient_interpolation = util_pedal_pico_interpolate(pedal_planets_coefficient_interpolation, pedal_planets_coefficient, PEDAL_PLANETS_COEFFICIENT_INTERPOLATION_ACCUM);
    pedal_planets_delay_time_interpolation = util_pedal_pico_interpolate(pedal_planets_delay_time_interpolation, pedal_planets_delay_time, pedal_planets_delay_time_interpolation_accum);
    int32 normalized_1 = (int32)pedal_planets_conversion_1 - (int32)util_pedal_pico_adc_middle_moving_average;
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_planets_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PLANETS_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    int16 delay_x = pedal_planets_delay_x[((pedal_planets_delay_index + PEDAL_PLANETS_DELAY_TIME_MAX) - (uint16)((int16)pedal_planets_delay_time_interpolation)) % PEDAL_PLANETS_DELAY_TIME_MAX];
    int16 delay_y = pedal_planets_delay_y[((pedal_planets_delay_index + PEDAL_PLANETS_DELAY_TIME_MAX) - (uint16)((int16)pedal_planets_delay_time_interpolation)) % PEDAL_PLANETS_DELAY_TIME_MAX];
    /* First Stage: High Pass Filter and Correction */
    int32 high_pass_1 = (int32)((int64)((((int64)delay_x << 16) * -(int64)pedal_planets_coefficient_interpolation) + (((int64)normalized_1 << 16) * (int64)(0x00010000 - pedal_planets_coefficient_interpolation))) >> 32);
    high_pass_1 = (int32)(int64)((((int64)high_pass_1 << 16) * (int64)pedal_planets_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(high_pass_1, PEDAL_PLANETS_PWM_PEAK))]) >> 32);
    /* Second Stage: Low Pass Filter to Sound from First Stage */
    if (util_pedal_pico_sw_mode == 1) high_pass_1 = normalized_1; // Bypass High Pass Filter
    int32 low_pass_1 = (int32)((int64)((((int64)delay_y << 16) * (int64)pedal_planets_coefficient_interpolation) + (((int64)high_pass_1 << 16) * (int64)(0x00010000 - pedal_planets_coefficient_interpolation))) >> 32);
    pedal_planets_delay_x[pedal_planets_delay_index] = (int16)high_pass_1;
    pedal_planets_delay_y[pedal_planets_delay_index] = (int16)low_pass_1;
    pedal_planets_delay_index++;
    if (pedal_planets_delay_index >= PEDAL_PLANETS_DELAY_TIME_MAX) pedal_planets_delay_index -= PEDAL_PLANETS_DELAY_TIME_MAX;
    int32 mixed_1;
    if (util_pedal_pico_sw_mode == 1) {
        mixed_1 = low_pass_1;
        pedal_planets_delay_time_interpolation_accum = PEDAL_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_2;
    } else if (util_pedal_pico_sw_mode == 2) {
        mixed_1 = high_pass_1;
        pedal_planets_delay_time_interpolation_accum = PEDAL_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_2;
    } else {
        mixed_1 = low_pass_1 << 1;
        pedal_planets_delay_time_interpolation_accum = PEDAL_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_1;
    }
    mixed_1 *= PEDAL_PLANETS_GAIN;
    /* Output */
    pedal_planets->output_1 = util_pedal_pico_cutoff_biased(mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_PLANETS_PWM_OFFSET + PEDAL_PLANETS_PWM_PEAK, PEDAL_PLANETS_PWM_OFFSET - PEDAL_PLANETS_PWM_PEAK);
    pedal_planets->output_1_inverted = util_pedal_pico_cutoff_biased(-mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_PLANETS_PWM_OFFSET + PEDAL_PLANETS_PWM_PEAK, PEDAL_PLANETS_PWM_OFFSET - PEDAL_PLANETS_PWM_PEAK);
}

void pedal_planets_free() { // Free Except Object, pedal_planets
    util_pedal_pico_stop((util_pedal_pico*)pedal_planets);
    irq_remove_handler(PWM_IRQ_WRAP, pedal_planets_on_pwm_irq_wrap);
    free((void*)pedal_planets_delay_x);
    free((void*)pedal_planets_delay_y);
    __dsb();
}
