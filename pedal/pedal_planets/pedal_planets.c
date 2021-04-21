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
// Private header
#include "pedal_planets.h"

#define PEDAL_PLANETS_CORE_1_STACK_SIZE 1024 * 4 // 1024 Words, 4096 Bytes
#define PEDAL_PLANETS_LED_GPIO 25
#define PEDAL_PLANETS_SWITCH_1_GPIO 14
#define PEDAL_PLANETS_SWITCH_2_GPIO 15
#define PEDAL_PLANETS_SWITCH_THRESHOLD 30
#define PEDAL_PLANETS_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_PLANETS_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_PLANETS_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_PLANETS_PWM_PEAK 2047
#define PEDAL_PLANETS_GAIN 1
#define PEDAL_PLANETS_OSC_SINE_1_TIME_MAX 61036
#define PEDAL_PLANETS_COEFFICIENT_FIXED_1 (int32)(0x0000F000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PLANETS_DELAY_TIME_MAX 1025 // Don't Use Delay Time = 0
#define PEDAL_PLANETS_DELAY_TIME_FIXED_1 513 // 30518 Divided by 513 (59.49Hz, Folding Frequency is 29.74Hz)
#define PEDAL_PLANETS_DELAY_TIME_SWING_PEAK_1 512 // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PLANETS_DELAY_TIME_SWING_SHIFT 4 // Multiply By 16 (1 - 32 to 16 - 512)
#define PEDAL_PLANETS_OSC_START_THRESHOLD_FIXED_1 8 // From -48.13dB (Loss 255) in ADC_VREF (Typically 3.3V)
#define PEDAL_PLANETS_OSC_START_COUNT_MAX 2000 // 30518 Divided by 4000 = Approx. 8Hz
#define PEDAL_PLANETS_ADC_0_GPIO 26
#define PEDAL_PLANETS_ADC_1_GPIO 27
#define PEDAL_PLANETS_ADC_2_GPIO 28
#define PEDAL_PLANETS_ADC_MIDDLE_DEFAULT 2048
#define PEDAL_PLANETS_ADC_MIDDLE_NUMBER_MOVING_AVERAGE 16384 // Should be Power of 2 Because of Processing Speed (Logical Shift Left on Division)
#define PEDAL_PLANETS_ADC_THRESHOLD 0x3F // Range is 0x0-0xFFF (0-4095) Divided by 0x80 (128) for 0x0-0x1F (0-31), (0x80 >> 1) - 1.

volatile uint32 pedal_planets_pwm_slice_num;
volatile uint32 pedal_planets_pwm_channel;
volatile uint16 pedal_planets_conversion_1;
volatile uint16 pedal_planets_conversion_2;
volatile uint16 pedal_planets_conversion_3;
volatile uint16 pedal_planets_conversion_1_temp;
volatile uint16 pedal_planets_conversion_2_temp;
volatile uint16 pedal_planets_conversion_3_temp;
volatile uchar8 pedal_planets_mode;
volatile int32 pedal_planets_coefficient;
volatile int16* pedal_planets_delay_x;
volatile int16* pedal_planets_delay_y;
volatile uint16 pedal_planets_delay_time;
volatile uint16 pedal_planets_delay_index;
volatile uint16 pedal_planets_delay_time_swing;
volatile uint16 pedal_planets_osc_sine_1_index;
volatile uint16 pedal_planets_osc_speed;
volatile char8 pedal_planets_osc_start_threshold;
volatile uint16 pedal_planets_osc_start_count;
volatile uint32 pedal_planets_adc_middle_moving_average;
volatile bool pedal_planets_is_outstanding_on_adc;
volatile uint32 pedal_planets_debug_time;

void pedal_planets_core_1();
void pedal_planets_on_pwm_irq_wrap();
void pedal_planets_on_adc_irq_fifo();

int main(void) {
    //stdio_init_all();
    //sleep_ms(2000); // Wait for Rediness of USB for Messages
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
        tight_loop_contents();
    }
    return 0;
}

void pedal_planets_core_1() {
    /* PWM Settings */
    gpio_set_function(PEDAL_PLANETS_PWM_1_GPIO, GPIO_FUNC_PWM); // GPIO16 = PWM8 A
    gpio_set_function(PEDAL_PLANETS_PWM_2_GPIO, GPIO_FUNC_PWM); // GPIO17 = PWM8 B
    pedal_planets_pwm_slice_num = pwm_gpio_to_slice_num(PEDAL_PLANETS_PWM_1_GPIO); // GPIO16 = PWM8
    pedal_planets_pwm_channel = pwm_gpio_to_channel(PEDAL_PLANETS_PWM_1_GPIO); // GPIO16 = A
    // Set IRQ and Handler for PWM
    pwm_clear_irq(pedal_planets_pwm_slice_num);
    pwm_set_irq_enabled(pedal_planets_pwm_slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_planets_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0xF0); // Higher Priority
    // PWM Configuration (Make Approx. 30518Hz from 125Mhz - 0.032768ms Cycle)
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    pwm_config_set_clkdiv(&config, 1.0f); // Set Clock Divider, 125,000,000 Divided by 1.0 for 0.008us Cycle
    pwm_config_set_wrap(&config, 4095); // 0-4095, 4096 Cycles for 0.032768ms
    pwm_init(pedal_planets_pwm_slice_num, &config, false); // Push Configufatio
    pwm_set_chan_level(pedal_planets_pwm_slice_num, pedal_planets_pwm_channel, PEDAL_PLANETS_PWM_OFFSET); // Set Channel A
    pwm_set_chan_level(pedal_planets_pwm_slice_num, pedal_planets_pwm_channel + 1, PEDAL_PLANETS_PWM_OFFSET); // Set Channel B
    /* ADC Settings */
    adc_init();
    adc_gpio_init(PEDAL_PLANETS_ADC_0_GPIO); // GPIO26 (ADC0)
    adc_gpio_init(PEDAL_PLANETS_ADC_1_GPIO); // GPIO27 (ADC1)
    adc_gpio_init(PEDAL_PLANETS_ADC_2_GPIO); // GPIO28 (ADC2)
    adc_set_clkdiv(0.0f);
    adc_set_round_robin(0b00111);
    adc_fifo_setup(true, false, 3, true, false); // 12-bit Length (0-4095), Bit[15] for Error Flag
    adc_fifo_drain(); // Clean FIFO
    irq_set_exclusive_handler(ADC_IRQ_FIFO, pedal_planets_on_adc_irq_fifo);
    irq_set_priority(ADC_IRQ_FIFO, 0xFF); // Highest Priority
    adc_irq_set_enabled(true);
    pedal_planets_conversion_1 = PEDAL_PLANETS_ADC_MIDDLE_DEFAULT;
    pedal_planets_conversion_2 = PEDAL_PLANETS_ADC_MIDDLE_DEFAULT;
    pedal_planets_conversion_3 = PEDAL_PLANETS_ADC_MIDDLE_DEFAULT;
    pedal_planets_conversion_1_temp = PEDAL_PLANETS_ADC_MIDDLE_DEFAULT;
    pedal_planets_conversion_2_temp = PEDAL_PLANETS_ADC_MIDDLE_DEFAULT;
    pedal_planets_conversion_3_temp = PEDAL_PLANETS_ADC_MIDDLE_DEFAULT;
    pedal_planets_adc_middle_moving_average = pedal_planets_conversion_1 * PEDAL_PLANETS_ADC_MIDDLE_NUMBER_MOVING_AVERAGE;
    pedal_planets_coefficient = PEDAL_PLANETS_COEFFICIENT_FIXED_1;
    pedal_planets_delay_x = (int16*)calloc(PEDAL_PLANETS_DELAY_TIME_MAX, sizeof(int16));
    pedal_planets_delay_y = (int16*)calloc(PEDAL_PLANETS_DELAY_TIME_MAX, sizeof(int16));
    pedal_planets_delay_time = PEDAL_PLANETS_DELAY_TIME_FIXED_1;
    pedal_planets_delay_time_swing = ((pedal_planets_conversion_3 >> 7) + 1) << PEDAL_PLANETS_DELAY_TIME_SWING_SHIFT; // Make 5-bit Value (0-31) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    pedal_planets_delay_index = 0;
    pedal_planets_osc_sine_1_index = 0;
    pedal_planets_osc_speed = pedal_planets_conversion_2 >> 7; // Make 5-bit Value (0-31)
    pedal_planets_osc_start_threshold = PEDAL_PLANETS_OSC_START_THRESHOLD_FIXED_1;
    pedal_planets_osc_start_count = 0;
    /* Start IRQ, PWM and ADC */
    irq_set_mask_enabled(0b1 << PWM_IRQ_WRAP|0b1 << ADC_IRQ_FIFO, true);
    pwm_set_mask_enabled(0b1 << pedal_planets_pwm_slice_num);
    pedal_planets_is_outstanding_on_adc = true;
    adc_select_input(0); // Ensure to Start from A0
    __dsb();
    __isb();
    adc_run(true);
    uint32 gpio_mask = 0b1<< PEDAL_PLANETS_SWITCH_1_GPIO|0b1 << PEDAL_PLANETS_SWITCH_2_GPIO;
    gpio_init_mask(gpio_mask);
    gpio_set_dir_masked(gpio_mask, 0b1 << PEDAL_PLANETS_LED_GPIO);
    gpio_put(PEDAL_PLANETS_LED_GPIO, true);
    gpio_pull_up(PEDAL_PLANETS_SWITCH_1_GPIO);
    gpio_pull_up(PEDAL_PLANETS_SWITCH_2_GPIO);
    uint16 count_switch_0 = 0; // Center
    uint16 count_switch_1 = 0;
    uint16 count_switch_2 = 0;
    uchar8 mode = 0; // To Reduce Memory Access
    while (true) {
        switch (gpio_get_all() & (0b1 << PEDAL_PLANETS_SWITCH_1_GPIO|0b1 << PEDAL_PLANETS_SWITCH_2_GPIO)) {
            case 0b1 << PEDAL_PLANETS_SWITCH_2_GPIO: // SWITCH_1: Low
                count_switch_0 = 0;
                count_switch_1++;
                count_switch_2 = 0;
                if (count_switch_1 >= PEDAL_PLANETS_SWITCH_THRESHOLD) {
                    count_switch_1 = 0;
                    if (mode != 1) {
                        pedal_planets_mode = 1;
                        mode = 1;
                    }
                }
                break;
            case 0b1 << PEDAL_PLANETS_SWITCH_1_GPIO: // SWITCH_2: Low
                count_switch_0 = 0;
                count_switch_1 = 0;
                count_switch_2++;
                if (count_switch_2 >= PEDAL_PLANETS_SWITCH_THRESHOLD) {
                    count_switch_2 = 0;
                    if (mode != 2) {
                        pedal_planets_mode = 2;
                        mode = 2;
                    }
                }
                break;
            default: // All High
                count_switch_0++;
                count_switch_1 = 0;
                count_switch_2 = 0;
                if (count_switch_0 >= PEDAL_PLANETS_SWITCH_THRESHOLD) {
                    count_switch_0 = 0;
                    if (mode != 0) {
                        pedal_planets_mode = 0;
                        mode = 0;
                    }
                }
                break;
        }
        //printf("@main 3 - pedal_planets_conversion_1 %0x\n", pedal_planets_conversion_1);
        //printf("@main 4 - pedal_planets_conversion_2 %0x\n", pedal_planets_conversion_2);
        //printf("@main 5 - pedal_planets_conversion_3 %0x\n", pedal_planets_conversion_3);
        //printf("@main 6 - multicore_fifo_pop_blocking() %d\n", multicore_fifo_pop_blocking());
        //printf("@main 7 - pedal_planets_debug_time %d\n", pedal_planets_debug_time);
        sleep_us(1000);
        __dsb();
    }
}

void pedal_planets_on_pwm_irq_wrap() {
    pwm_clear_irq(pedal_planets_pwm_slice_num);
    //uint32 from_time = time_us_32();
    uint16 conversion_1_temp = pedal_planets_conversion_1_temp;
    uint16 conversion_2_temp = pedal_planets_conversion_2_temp;
    uint16 conversion_3_temp = pedal_planets_conversion_3_temp;
    if (! pedal_planets_is_outstanding_on_adc) {
        pedal_planets_is_outstanding_on_adc = true;
        adc_select_input(0); // Ensure to Start from ADC0
        __dsb();
        __isb();
        adc_run(true); // Stable Starting Point after PWM IRQ
    }
    pedal_planets_conversion_1 = conversion_1_temp;
    if (abs(conversion_2_temp - pedal_planets_conversion_2) > PEDAL_PLANETS_ADC_THRESHOLD) {
        pedal_planets_conversion_2 = conversion_2_temp;
        pedal_planets_osc_speed = pedal_planets_conversion_2 >> 7; // Make 5-bit Value (0-31)
    }
    if (abs(conversion_3_temp - pedal_planets_conversion_3) > PEDAL_PLANETS_ADC_THRESHOLD) {
        pedal_planets_conversion_3 = conversion_3_temp;
        pedal_planets_delay_time_swing = ((pedal_planets_conversion_3 >> 7) + 1) << PEDAL_PLANETS_DELAY_TIME_SWING_SHIFT; // Make 5-bit Value (0-31) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    }
    uint32 middle_moving_average = pedal_planets_adc_middle_moving_average / PEDAL_PLANETS_ADC_MIDDLE_NUMBER_MOVING_AVERAGE;
    pedal_planets_adc_middle_moving_average -= middle_moving_average;
    pedal_planets_adc_middle_moving_average += pedal_planets_conversion_1;
    int32 normalized_1 = (int32)pedal_planets_conversion_1 - (int32)middle_moving_average;
    /**
     * pedal_planets_osc_start_count:
     *
     * Over Positive Threshold       ## 1
     *-----------------------------------------------------------------------------------------------------------
     * Under Positive Threshold     # 0 # 2      ### Reset to 1
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                  # 0   # 3   # 5   # 2
     *-----------------------------------------------------------------------------------------------------------
     * 0                           # 0   # 4   # 4   # 3   # 5 ...Count Up to PEDAL_PLANETS_OSC_START_COUNT_MAX
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                         # 5 # 3      #### 4
     *-----------------------------------------------------------------------------------------------------------
     * Under Negative Threshold           # 6 # 2
     *-----------------------------------------------------------------------------------------------------------
     * Over Negative Threshold             ## Reset to 1
     */
    if (normalized_1 > pedal_planets_osc_start_threshold || normalized_1 < -pedal_planets_osc_start_threshold) {
        pedal_planets_osc_start_count = 1;
    } else if (pedal_planets_osc_start_count != 0 && (normalized_1 > (pedal_planets_osc_start_threshold >> 1) || normalized_1 < -(pedal_planets_osc_start_threshold >> 1))) {
        pedal_planets_osc_start_count = 1;
    } else if (pedal_planets_osc_start_count != 0) {
        pedal_planets_osc_start_count++;
    }
    if (pedal_planets_osc_start_count >= PEDAL_PLANETS_OSC_START_COUNT_MAX) pedal_planets_osc_start_count = 0;
    if (pedal_planets_osc_start_count == 0) {
        pedal_planets_osc_sine_1_index = 0;
    }
    /* Get Oscillator */
    int32 fixed_point_value_sine_1 = pedal_planets_table_sine_1[pedal_planets_osc_sine_1_index];
    pedal_planets_osc_sine_1_index += pedal_planets_osc_speed;
    if (pedal_planets_osc_sine_1_index >= PEDAL_PLANETS_OSC_SINE_1_TIME_MAX) pedal_planets_osc_sine_1_index -= PEDAL_PLANETS_OSC_SINE_1_TIME_MAX;
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_planets_table_pdf_1[abs(_cutoff_normalized(normalized_1, PEDAL_PLANETS_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    int16 delay_time_swing = (int16)(int64)(((int64)(pedal_planets_delay_time_swing << 16) * (int64)fixed_point_value_sine_1) >> 32);
    int16 delay_x = pedal_planets_delay_x[((pedal_planets_delay_index + PEDAL_PLANETS_DELAY_TIME_MAX) - (uint16)((int16)pedal_planets_delay_time + delay_time_swing)) % PEDAL_PLANETS_DELAY_TIME_MAX];
    int16 delay_y = pedal_planets_delay_y[((pedal_planets_delay_index + PEDAL_PLANETS_DELAY_TIME_MAX) - (uint16)((int16)pedal_planets_delay_time + delay_time_swing)) % PEDAL_PLANETS_DELAY_TIME_MAX];
    /* High Pass Filter and Correction */
    int32 high_pass_1 = (int32)((int64)((((int64)delay_x << 16) * (int64)pedal_planets_coefficient) + (((int64)normalized_1 << 16) * (int64)(0x00010000 - pedal_planets_coefficient))) >> 32);
    high_pass_1 = (int32)(int64)((((int64)high_pass_1 << 16) * (int64)pedal_planets_table_pdf_1[abs(_cutoff_normalized(high_pass_1, PEDAL_PLANETS_PWM_PEAK))]) >> 32);
    /* Low Pass Filster */
    int32 low_pass_1 = (int32)((int64)((((int64)delay_y << 16) * (int64)pedal_planets_coefficient) + (((int64)high_pass_1 << 16) * (int64)(0x00010000 - pedal_planets_coefficient))) >> 32);
    pedal_planets_delay_x[pedal_planets_delay_index] = (int16)normalized_1;
    pedal_planets_delay_y[pedal_planets_delay_index] = (int16)low_pass_1;
    pedal_planets_delay_index++;
    if (pedal_planets_delay_index >= PEDAL_PLANETS_DELAY_TIME_MAX) pedal_planets_delay_index = 0;
    int32 mixed_1;
    if (pedal_planets_mode == 0) {
        mixed_1 = low_pass_1;
    } else if (pedal_planets_mode == 1) {
        mixed_1 = high_pass_1;
    } else {
        mixed_1 = (normalized_1 + low_pass_1) >> 1;
    }
    mixed_1 *= PEDAL_PLANETS_GAIN;
    int32 output_1 = _cutoff_biased(mixed_1 + middle_moving_average, PEDAL_PLANETS_PWM_OFFSET + PEDAL_PLANETS_PWM_PEAK, PEDAL_PLANETS_PWM_OFFSET - PEDAL_PLANETS_PWM_PEAK);
    int32 output_1_inverted = _cutoff_biased(-mixed_1 + middle_moving_average, PEDAL_PLANETS_PWM_OFFSET + PEDAL_PLANETS_PWM_PEAK, PEDAL_PLANETS_PWM_OFFSET - PEDAL_PLANETS_PWM_PEAK);
    pwm_set_chan_level(pedal_planets_pwm_slice_num, pedal_planets_pwm_channel, (uint16)output_1);
    pwm_set_chan_level(pedal_planets_pwm_slice_num, pedal_planets_pwm_channel + 1, (uint16)output_1_inverted);
    //pedal_planets_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_planets_debug_time); // To send a made pointer, sync flag, etc.
    __dsb();
}

void pedal_planets_on_adc_irq_fifo() {
    adc_run(false);
    __dsb();
    __isb();
    uint16 adc_fifo_level = adc_fifo_get_level(); // Seems 8 at Maximum
    //printf("@pedal_planets_on_adc_irq_fifo 1 - adc_fifo_level: %d\n", adc_fifo_level);
    for (uint16 i = 0; i < adc_fifo_level; i++) {
        //printf("@pedal_planets_on_adc_irq_fifo 2 - i: %d\n", i);
        uint16 temp = adc_fifo_get();
        if (temp & 0x8000) { // Procedure on Malfunction
            reset_block(RESETS_RESET_PWM_BITS|RESETS_RESET_ADC_BITS);
            break;
        } else {
            temp &= 0x7FFF; // Clear Bit[15]: ERR
            uint16 remainder = i % 3;
            if (remainder == 2) {
                pedal_planets_conversion_3_temp = temp;
            } else if (remainder == 1) {
                pedal_planets_conversion_2_temp = temp;
            } else if (remainder == 0) {
               pedal_planets_conversion_1_temp = temp;
            }
        }
    }
    //printf("@pedal_planets_on_adc_irq_fifo 3 - adc_fifo_is_empty(): %d\n", adc_fifo_is_empty());
    adc_fifo_drain();
    do {
        tight_loop_contents();
    } while (! adc_fifo_is_empty);
    pedal_planets_is_outstanding_on_adc = false;
    __dsb();
}
