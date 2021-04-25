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
#include "pedal_tape.h"

#define PEDAL_TAPE_TRANSIENT_RESPONSE 100000 // 100000 Micro Seconds
#define PEDAL_TAPE_CORE_1_STACK_SIZE 1024 * 4 // 1024 Words, 4096 Bytes
#define PEDAL_TAPE_LED_GPIO 25
#define PEDAL_TAPE_SW_1_GPIO 14
#define PEDAL_TAPE_SW_2_GPIO 15
#define PEDAL_TAPE_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_TAPE_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_TAPE_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_TAPE_PWM_PEAK 2047
#define PEDAL_TAPE_GAIN 1
#define PEDAL_TAPE_DELAY_AMPLITUDE_PEAK_FIXED_1 (int32)(0x00008000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_TAPE_DELAY_TIME_MAX 3969
#define PEDAL_TAPE_DELAY_TIME_FIXED_1 1984 // 1920 Divided by 30518 (0.063 Seconds)
#define PEDAL_TAPE_DELAY_TIME_SWING_PEAK_1 1984
#define PEDAL_TAPE_DELAY_TIME_SWING_SHIFT 6 // Multiply By 64 (0-1984)
#define PEDAL_TAPE_OSC_SINE_1_TIME_MAX 30518
#define PEDAL_TAPE_ADC_MIDDLE_DEFAULT 2048
#define PEDAL_TAPE_ADC_MIDDLE_NUMBER_MOVING_AVERAGE 16384 // Should be Power of 2 Because of Processing Speed (Logical Shift Left on Division)
#define PEDAL_TAPE_ADC_THRESHOLD 0x3F // Range is 0x0-0xFFF (0-4095) Divided by 0x80 (128) for 0x0-0x1F (0-31), (0x80 >> 1) - 1.

volatile uint32 pedal_tape_pwm_slice_num;
volatile uint32 pedal_tape_pwm_channel;
volatile uint16 pedal_tape_conversion_1;
volatile uint16 pedal_tape_conversion_2;
volatile uint16 pedal_tape_conversion_3;
volatile uint16 pedal_tape_osc_sine_1_index;
volatile uint32 pedal_tape_osc_speed;
volatile int16* pedal_tape_delay_array;
volatile int32 pedal_tape_delay_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
volatile uint16 pedal_tape_delay_time;
volatile uint16 pedal_tape_delay_index;
volatile uint16 pedal_tape_delay_time_swing;
volatile uint32 pedal_tape_adc_middle_moving_average;
volatile uint32 pedal_tape_debug_time;

void pedal_tape_core_1();
void pedal_tape_on_pwm_irq_wrap();

int main(void) {
    //stdio_init_all();
    //sleep_ms(2000); // Wait for Rediness of USB for Messages
    sleep_us(PEDAL_TAPE_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(PEDAL_TAPE_LED_GPIO);
    gpio_set_dir(PEDAL_TAPE_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_TAPE_LED_GPIO, true);
    uint32* stack_pointer = (int32*)malloc(PEDAL_TAPE_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(pedal_tape_core_1, stack_pointer, PEDAL_TAPE_CORE_1_STACK_SIZE);
    //pedal_tape_debug_time = 0;
    //uint32 from_time = time_us_32();
    //printf("@main 1 - Let's Start!\n");
    //pedal_tape_debug_time = time_us_32() - from_time;
    //printf("@main 2 - pedal_tape_debug_time %d\n", pedal_tape_debug_time);
    while (true) {
        //printf("@main 3 - pedal_tape_conversion_1 %0x\n", pedal_tape_conversion_1);
        //printf("@main 4 - pedal_tape_conversion_2 %0x\n", pedal_tape_conversion_2);
        //printf("@main 5 - pedal_tape_conversion_3 %0x\n", pedal_tape_conversion_3);
        //printf("@main 6 - multicore_fifo_pop_blocking() %d\n", multicore_fifo_pop_blocking());
        //printf("@main 7 - pedal_tape_debug_time %d\n", pedal_tape_debug_time);
        //sleep_ms(500);
        //tight_loop_contents();
        __wfi();
    }
    return 0;
}

void pedal_tape_core_1() {
    /* PWM Settings */
    gpio_set_function(PEDAL_TAPE_PWM_1_GPIO, GPIO_FUNC_PWM); // GPIO16 = PWM8 A
    gpio_set_function(PEDAL_TAPE_PWM_2_GPIO, GPIO_FUNC_PWM); // GPIO17 = PWM8 B
    pedal_tape_pwm_slice_num = pwm_gpio_to_slice_num(PEDAL_TAPE_PWM_1_GPIO); // GPIO16 = PWM8
    pedal_tape_pwm_channel = pwm_gpio_to_channel(PEDAL_TAPE_PWM_1_GPIO); // GPIO16 = A
    // Set IRQ and Handler for PWM
    pwm_clear_irq(pedal_tape_pwm_slice_num);
    pwm_set_irq_enabled(pedal_tape_pwm_slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_tape_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0xF0); // Higher Priority
    // PWM Configuration (Make Approx. 30518Hz from 125Mhz - 0.032768ms Cycle)
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    pwm_config_set_clkdiv(&config, 1.0f); // Set Clock Divider, 125,000,000 Divided by 1.0 for 0.008us Cycle
    pwm_config_set_wrap(&config, 4095); // 0-4095, 4096 Cycles for 0.032768ms
    pwm_init(pedal_tape_pwm_slice_num, &config, false); // Push Configufatio
    pwm_set_chan_level(pedal_tape_pwm_slice_num, pedal_tape_pwm_channel, PEDAL_TAPE_PWM_OFFSET); // Set Channel A
    pwm_set_chan_level(pedal_tape_pwm_slice_num, pedal_tape_pwm_channel + 1, PEDAL_TAPE_PWM_OFFSET); // Set Channel B
    /* ADC Settings */
    util_pedal_pico_init_adc();
    util_pedal_pico_on_adc_conversion_1 = PEDAL_TAPE_ADC_MIDDLE_DEFAULT;
    util_pedal_pico_on_adc_conversion_2 = PEDAL_TAPE_ADC_MIDDLE_DEFAULT;
    util_pedal_pico_on_adc_conversion_3 = PEDAL_TAPE_ADC_MIDDLE_DEFAULT;
    pedal_tape_conversion_1 = PEDAL_TAPE_ADC_MIDDLE_DEFAULT;
    pedal_tape_conversion_2 = PEDAL_TAPE_ADC_MIDDLE_DEFAULT;
    pedal_tape_conversion_3 = PEDAL_TAPE_ADC_MIDDLE_DEFAULT;
    pedal_tape_adc_middle_moving_average = pedal_tape_conversion_1 * PEDAL_TAPE_ADC_MIDDLE_NUMBER_MOVING_AVERAGE;
    pedal_tape_delay_array = (int16*)calloc(PEDAL_TAPE_DELAY_TIME_MAX, sizeof(int16));
    pedal_tape_delay_amplitude = PEDAL_TAPE_DELAY_AMPLITUDE_PEAK_FIXED_1;
    pedal_tape_delay_time = PEDAL_TAPE_DELAY_TIME_FIXED_1;
    pedal_tape_delay_time_swing = (pedal_tape_conversion_2 >> 7) << PEDAL_TAPE_DELAY_TIME_SWING_SHIFT; // Make 5-bit Value (0-31) and Multiply
    pedal_tape_delay_index = 0;
    pedal_tape_osc_speed = pedal_tape_conversion_3 >> 7; // Make 5-bit Value (0-31)
    pedal_tape_osc_sine_1_index = 0;
    /* Start IRQ, PWM and ADC */
    util_pedal_pico_sw_mode = 0; // Initialize Mode of Switch Before Running PWM and ADC
    irq_set_mask_enabled(0b1 << PWM_IRQ_WRAP|0b1 << ADC_IRQ_FIFO, true);
    pwm_set_mask_enabled(0b1 << pedal_tape_pwm_slice_num);
    adc_select_input(0); // Ensure to Start from A0
    __dsb();
    __isb();
    adc_run(true);
    util_pedal_pico_sw_loop(PEDAL_TAPE_SW_1_GPIO, PEDAL_TAPE_SW_2_GPIO);
}

void pedal_tape_on_pwm_irq_wrap() {
    pwm_clear_irq(pedal_tape_pwm_slice_num);
    //uint32 from_time = time_us_32();
    /* Input */
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
    pedal_tape_conversion_1 = conversion_1_temp;
    if (abs(conversion_2_temp - pedal_tape_conversion_2) > PEDAL_TAPE_ADC_THRESHOLD) {
        pedal_tape_conversion_2 = conversion_2_temp;
        pedal_tape_delay_time_swing = (pedal_tape_conversion_2 >> 7) << PEDAL_TAPE_DELAY_TIME_SWING_SHIFT; // Make 5-bit Value (0-31) and Multiply
    }
    if (abs(conversion_3_temp - pedal_tape_conversion_3) > PEDAL_TAPE_ADC_THRESHOLD) {
        pedal_tape_conversion_3 = conversion_3_temp;
        pedal_tape_osc_speed = pedal_tape_conversion_3 >> 7; // Make 5-bit Value (0-31)
    }
    uint32 middle_moving_average = pedal_tape_adc_middle_moving_average / PEDAL_TAPE_ADC_MIDDLE_NUMBER_MOVING_AVERAGE;
    pedal_tape_adc_middle_moving_average -= middle_moving_average;
    pedal_tape_adc_middle_moving_average += pedal_tape_conversion_1;
    int32 normalized_1 = (int32)pedal_tape_conversion_1 - (int32)middle_moving_average;
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_tape_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_TAPE_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    /* Get Oscillator */
    int32 fixed_point_value_sine_1 = pedal_tape_table_sine_1[pedal_tape_osc_sine_1_index];
    pedal_tape_osc_sine_1_index += pedal_tape_osc_speed;
    if (pedal_tape_osc_sine_1_index >= PEDAL_TAPE_OSC_SINE_1_TIME_MAX) pedal_tape_osc_sine_1_index = 0;
    int16 time_swing = (int16)(int64)((((int64)pedal_tape_delay_time_swing << 16) * (int64)fixed_point_value_sine_1) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    int32 delay_1 = (int32)pedal_tape_delay_array[((pedal_tape_delay_index + PEDAL_TAPE_DELAY_TIME_MAX) - ((int16)pedal_tape_delay_time + time_swing)) % PEDAL_TAPE_DELAY_TIME_MAX];
    if (pedal_tape_delay_time + time_swing == 0) delay_1 = 0; // No Delay, Otherwise Latest
    int32 pedal_tape_normalized_1_amplitude = 0x00010000 - pedal_tape_delay_amplitude;
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_tape_normalized_1_amplitude) >> 32);
    delay_1 = (int32)(int64)((((int64)delay_1 << 16) * (int64)pedal_tape_delay_amplitude) >> 32);
    int32 mixed_1 = normalized_1 + delay_1;
    pedal_tape_delay_array[pedal_tape_delay_index] = (int16)mixed_1;
    pedal_tape_delay_index++;
    if (pedal_tape_delay_index >= PEDAL_TAPE_DELAY_TIME_MAX) pedal_tape_delay_index -= PEDAL_TAPE_DELAY_TIME_MAX;
    mixed_1 *= PEDAL_TAPE_GAIN;
    /* Output */
    int32 output_1 = util_pedal_pico_cutoff_biased(mixed_1 * PEDAL_TAPE_GAIN + middle_moving_average, PEDAL_TAPE_PWM_OFFSET + PEDAL_TAPE_PWM_PEAK, PEDAL_TAPE_PWM_OFFSET - PEDAL_TAPE_PWM_PEAK);
    int32 output_1_inverted = util_pedal_pico_cutoff_biased(-mixed_1 * PEDAL_TAPE_GAIN + middle_moving_average, PEDAL_TAPE_PWM_OFFSET + PEDAL_TAPE_PWM_PEAK, PEDAL_TAPE_PWM_OFFSET - PEDAL_TAPE_PWM_PEAK);
    pwm_set_chan_level(pedal_tape_pwm_slice_num, pedal_tape_pwm_channel, (uint16)output_1);
    pwm_set_chan_level(pedal_tape_pwm_slice_num, pedal_tape_pwm_channel + 1, (uint16)output_1_inverted);
    //pedal_tape_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_tape_debug_time); // To send a made pointer, sync flag, etc.
    __dsb();
}
