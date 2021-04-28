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
#include "pedal_distortion.h"

#define PEDAL_DISTORTION_TRANSIENT_RESPONSE 100000 // 100000 Micro Seconds
#define PEDAL_DISTORTION_CORE_1_STACK_SIZE 1024 * 4 // 1024 Words, 4096 Bytes
#define PEDAL_DISTORTION_LED_GPIO 25
#define PEDAL_DISTORTION_SW_1_GPIO 14
#define PEDAL_DISTORTION_SW_2_GPIO 15
#define PEDAL_DISTORTION_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_DISTORTION_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_DISTORTION_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_DISTORTION_PWM_PEAK 2047
#define PEDAL_DISTORTION_GAIN 1
#define PEDAL_DISTORTION_ADC_MIDDLE_DEFAULT 2048
#define PEDAL_DISTORTION_ADC_MIDDLE_NUMBER_MOVING_AVERAGE 16384 // Should be Power of 2 Because of Processing Speed (Logical Shift Left on Division)
#define PEDAL_DISTORTION_ADC_THRESHOLD 0x3F // Range is 0x0-0xFFF (0-4095) Divided by 0x80 (128) for 0x0-0x1F (0-31), (0x80 >> 1) - 1.
#define PEDAL_DISTORTION_CUTOFF_FIXED_1 0xC0

volatile uint32 pedal_distortion_pwm_slice_num;
volatile uint32 pedal_distortion_pwm_channel;
volatile uint16 pedal_distortion_conversion_1;
volatile uint16 pedal_distortion_conversion_2;
volatile uint16 pedal_distortion_conversion_3;
volatile uint16 pedal_distortion_loss;
volatile uint32 pedal_distortion_adc_middle_moving_average;
volatile uint32 pedal_distortion_debug_time;

void pedal_distortion_core_1();
void pedal_distortion_on_pwm_irq_wrap();

int main(void) {
    //stdio_init_all();
    util_pedal_pico_set_sys_clock_115200khz();
    //stdio_init_all(); // Re-init for UART Baud Rate
    sleep_us(PEDAL_DISTORTION_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(PEDAL_DISTORTION_LED_GPIO);
    gpio_set_dir(PEDAL_DISTORTION_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_DISTORTION_LED_GPIO, true);
    uint32* stack_pointer = (int32*)malloc(PEDAL_DISTORTION_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(pedal_distortion_core_1, stack_pointer, PEDAL_DISTORTION_CORE_1_STACK_SIZE);
    //pedal_distortion_debug_time = 0;
    //uint32 from_time = time_us_32();
    //printf("@main 1 - Let's Start!\n");
    //pedal_distortion_debug_time = time_us_32() - from_time;
    //printf("@main 2 - pedal_distortion_debug_time %d\n", pedal_distortion_debug_time);
    while (true) {
        //printf("@main 3 - pedal_distortion_conversion_1 %0x\n", pedal_distortion_conversion_1);
        //printf("@main 4 - pedal_distortion_conversion_2 %0x\n", pedal_distortion_conversion_2);
        //printf("@main 5 - pedal_distortion_conversion_3 %0x\n", pedal_distortion_conversion_3);
        //printf("@main 6 - multicore_fifo_pop_blocking() %d\n", multicore_fifo_pop_blocking());
        //printf("@main 7 - pedal_distortion_debug_time %d\n", pedal_distortion_debug_time);
        //sleep_ms(500);
        //tight_loop_contents();
        __wfi();
    }
    return 0;
}

void pedal_distortion_core_1() {
    /* PWM Settings */
    gpio_set_function(PEDAL_DISTORTION_PWM_1_GPIO, GPIO_FUNC_PWM); // GPIO16 = PWM8 A
    gpio_set_function(PEDAL_DISTORTION_PWM_2_GPIO, GPIO_FUNC_PWM); // GPIO17 = PWM8 B
    pedal_distortion_pwm_slice_num = pwm_gpio_to_slice_num(PEDAL_DISTORTION_PWM_1_GPIO); // GPIO16 = PWM8
    pedal_distortion_pwm_channel = pwm_gpio_to_channel(PEDAL_DISTORTION_PWM_1_GPIO); // GPIO16 = A
    // Set IRQ and Handler for PWM
    pwm_clear_irq(pedal_distortion_pwm_slice_num);
    pwm_set_irq_enabled(pedal_distortion_pwm_slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_distortion_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0xF0); // Higher Priority
    // PWM Configuration
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    util_pedal_pico_set_pwm_28125hz(&config);
    pwm_init(pedal_distortion_pwm_slice_num, &config, false); // Push Configufatio
    pwm_set_chan_level(pedal_distortion_pwm_slice_num, pedal_distortion_pwm_channel, PEDAL_DISTORTION_PWM_OFFSET); // Set Channel A
    pwm_set_chan_level(pedal_distortion_pwm_slice_num, pedal_distortion_pwm_channel + 1, PEDAL_DISTORTION_PWM_OFFSET); // Set Channel B
    /* ADC Settings */
    util_pedal_pico_init_adc();
    util_pedal_pico_on_adc_conversion_1 = PEDAL_DISTORTION_ADC_MIDDLE_DEFAULT;
    util_pedal_pico_on_adc_conversion_2 = PEDAL_DISTORTION_ADC_MIDDLE_DEFAULT;
    util_pedal_pico_on_adc_conversion_3 = PEDAL_DISTORTION_ADC_MIDDLE_DEFAULT;
    pedal_distortion_conversion_1 = PEDAL_DISTORTION_ADC_MIDDLE_DEFAULT;
    pedal_distortion_conversion_2 = PEDAL_DISTORTION_ADC_MIDDLE_DEFAULT;
    pedal_distortion_conversion_3 = PEDAL_DISTORTION_ADC_MIDDLE_DEFAULT;
    pedal_distortion_adc_middle_moving_average = pedal_distortion_conversion_1 * PEDAL_DISTORTION_ADC_MIDDLE_NUMBER_MOVING_AVERAGE;
    pedal_distortion_loss = 32 - (pedal_distortion_conversion_2 >> 7); // Make 5-bit Value (1-32)
    /* Start IRQ, PWM and ADC */
    util_pedal_pico_sw_mode = 0; // Initialize Mode of Switch Before Running PWM and ADC
    irq_set_mask_enabled(0b1 << PWM_IRQ_WRAP|0b1 << ADC_IRQ_FIFO, true);
    pwm_set_mask_enabled(0b1 << pedal_distortion_pwm_slice_num);
    adc_select_input(0); // Ensure to Start from A0
    __dsb();
    __isb();
    adc_run(true);
    util_pedal_pico_sw_loop(PEDAL_DISTORTION_SW_1_GPIO, PEDAL_DISTORTION_SW_2_GPIO);
}

void pedal_distortion_on_pwm_irq_wrap() {
    pwm_clear_irq(pedal_distortion_pwm_slice_num);
    //uint32 from_time = time_us_32();
    uint16 conversion_1_temp = util_pedal_pico_on_adc_conversion_1;
    uint16 conversion_2_temp = util_pedal_pico_on_adc_conversion_2;
    uint16 conversion_3_temp = util_pedal_pico_on_adc_conversion_3;
    if (! util_pedal_pico_on_adc_is_outstanding) {
        util_pedal_pico_on_adc_is_outstanding = true;
        adc_select_input(0); // Ensure to Start from A0
        __dsb();
        __isb();
        adc_run(true); // Stable Starting Point after PWM IRQ
    }
    pedal_distortion_conversion_1 = conversion_1_temp;
    if (abs(conversion_2_temp - pedal_distortion_conversion_2) > PEDAL_DISTORTION_ADC_THRESHOLD) {
        pedal_distortion_conversion_2 = conversion_2_temp;
        pedal_distortion_loss = 32 - (pedal_distortion_conversion_2 >> 7); // Make 5-bit Value (1-32)
    }
    if (abs(conversion_3_temp - pedal_distortion_conversion_3) > PEDAL_DISTORTION_ADC_THRESHOLD) {
        pedal_distortion_conversion_3 = conversion_3_temp;
    }
    uint32 middle_moving_average = pedal_distortion_adc_middle_moving_average / PEDAL_DISTORTION_ADC_MIDDLE_NUMBER_MOVING_AVERAGE;
    pedal_distortion_adc_middle_moving_average -= middle_moving_average;
    pedal_distortion_adc_middle_moving_average += pedal_distortion_conversion_1;
    int32 normalized_1 = (int32)pedal_distortion_conversion_1 - (int32)middle_moving_average;
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_distortion_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_DISTORTION_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    normalized_1 = util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_DISTORTION_CUTOFF_FIXED_1);
    if (util_pedal_pico_sw_mode == 1) {
        if (normalized_1 > 0) {
            normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_distortion_table_log_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_DISTORTION_PWM_PEAK))]) >> 32);
        } else {
            normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_distortion_table_log_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_DISTORTION_PWM_PEAK))]) >> 32);
        }
    } else if (util_pedal_pico_sw_mode == 2) {
        if (normalized_1 > 0) {
            normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_distortion_table_log_2[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_DISTORTION_PWM_PEAK))]) >> 32);
        } else {
            normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_distortion_table_log_2[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_DISTORTION_PWM_PEAK))]) >> 32);
        }
    } else {
        if (normalized_1 > 0) {
            normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_distortion_table_log_2[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_DISTORTION_PWM_PEAK))]) >> 32);
        } else {
            normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_distortion_table_power_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_DISTORTION_PWM_PEAK))]) >> 32);
        }
    }
    normalized_1 /= pedal_distortion_loss;
    normalized_1 *= PEDAL_DISTORTION_GAIN;
    int32 output_1 = util_pedal_pico_cutoff_biased(normalized_1 + middle_moving_average, PEDAL_DISTORTION_PWM_OFFSET + PEDAL_DISTORTION_PWM_PEAK, PEDAL_DISTORTION_PWM_OFFSET - PEDAL_DISTORTION_PWM_PEAK);
    int32 output_1_inverted = util_pedal_pico_cutoff_biased(-normalized_1 + middle_moving_average, PEDAL_DISTORTION_PWM_OFFSET + PEDAL_DISTORTION_PWM_PEAK, PEDAL_DISTORTION_PWM_OFFSET - PEDAL_DISTORTION_PWM_PEAK);
    pwm_set_chan_level(pedal_distortion_pwm_slice_num, pedal_distortion_pwm_channel, (uint16)output_1);
    pwm_set_chan_level(pedal_distortion_pwm_slice_num, pedal_distortion_pwm_channel + 1, (uint16)output_1_inverted);
    //pedal_distortion_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_distortion_debug_time); // To send a made pointer, sync flag, etc.
    __dsb();
}
