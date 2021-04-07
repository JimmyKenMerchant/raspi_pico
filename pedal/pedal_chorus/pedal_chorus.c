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
#include "pico/float.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
// raspi_pico/include
#include "macros_pico.h"
//#include "function_generator_pico.h"
// Private header
#include "pedal_chorus.h"

#define PEDAL_CHORUS_LED_GPIO 25
#define PEDAL_CHORUS_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_CHORUS_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_CHORUS_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_CHORUS_PWM_PEAK 2047
#define PEDAL_CHORUS_OSC_SINE_1_TIME_MAX 30518
#define PEDAL_CHORUS_OSC_SINE_2_TIME_MAX 15258
#define PEDAL_CHORUS_OSC_AMPLITUDE_PEAK 4095
#define PEDAL_CHORUS_NOISE_GATE_THRESHOLD_MULTIPLIER 2 // From -60.2dB (Loss 1024) to -36.7dB (Loss 68) in ADC_VREF (Typically 3.3V)
#define PEDAL_CHORUS_NOISE_GATE_COUNT_MAX 2000 // 30518 Divided by 2000 = Approx. 15Hz
#define PEDAL_CHORUS_ADC_0_GPIO 26
#define PEDAL_CHORUS_ADC_1_GPIO 27
#define PEDAL_CHORUS_ADC_2_GPIO 28
#define PEDAL_CHORUS_ADC_MIDDLE_DEFAULT 2048
#define PEDAL_CHORUS_ADC_MIDDLE_NUMBER_MOVING_AVERAGE 16384 // Should be Power of 2 Because of Processing Speed (Logical Shift Left on Division)
#define PEDAL_CHORUS_ADC_THRESHOLD 0x7F // Range is 0x0-0xFFF (0-4095) Divided by 0xFF (255) for 0x0-0xFb (0-15). 0xFF >> 1.

//function_generator_pico* function_generator;

uint32 pedal_chorus_pwm_slice_num;
uint32 pedal_chorus_pwm_channel;
uint16 pedal_chorus_conversion_1;
uint16 pedal_chorus_conversion_2;
uint16 pedal_chorus_conversion_3;
uint16 pedal_chorus_conversion_1_temp;
uint16 pedal_chorus_conversion_2_temp;
uint16 pedal_chorus_conversion_3_temp;
uint16 pedal_chorus_osc_sine_1_time;
uint16 pedal_chorus_osc_sine_2_time;
uint16 pedal_chorus_osc_amplitude;
uint16 pedal_chorus_osc_speed;
char8 pedal_chorus_gain;
char8 pedal_chorus_noise_gate_threshold;
uint16 pedal_chorus_noise_gate_count;
uint32 pedal_chorus_adc_middle_moving_average;
bool pedal_chorus_is_outstanding_on_adc;
uint32 pedal_chorus_debug_time;

void pedal_chorus_core_1();
void pedal_chorus_on_pwm_irq_wrap();
void pedal_chorus_on_adc_irq_fifo();

int main(void) {
    //stdio_init_all();
    //sleep_ms(2000); // Wait for Rediness of USB for Messages
    gpio_init(PEDAL_CHORUS_LED_GPIO);
    gpio_set_dir(PEDAL_CHORUS_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_CHORUS_LED_GPIO, true);
    multicore_launch_core1(pedal_chorus_core_1);
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
        tight_loop_contents();
    }
    return 0;
}

void pedal_chorus_core_1() {
    /* PWM Settings */
    gpio_set_function(PEDAL_CHORUS_PWM_1_GPIO, GPIO_FUNC_PWM); // GPIO16 = PWM8 A
    gpio_set_function(PEDAL_CHORUS_PWM_2_GPIO, GPIO_FUNC_PWM); // GPIO17 = PWM8 B
    pedal_chorus_pwm_slice_num = pwm_gpio_to_slice_num(PEDAL_CHORUS_PWM_1_GPIO); // GPIO16 = PWM8
    pedal_chorus_pwm_channel = pwm_gpio_to_channel(PEDAL_CHORUS_PWM_1_GPIO); // GPIO16 = A
    // Set IRQ and Handler for PWM
    pwm_clear_irq(pedal_chorus_pwm_slice_num);
    pwm_set_irq_enabled(pedal_chorus_pwm_slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_chorus_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0x80); // Middle Priority
    // PWM Configuration (Make Approx. 30518Hz from 125Mhz - 0.032768ms Cycle)
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    pwm_config_set_clkdiv(&config, 1.0f); // Set Clock Divider, 125,000,000 Divided by 1.0 for 0.008us Cycle
    pwm_config_set_wrap(&config, 4095); // 0-4095, 4096 Cycles for 0.032768ms
    pwm_init(pedal_chorus_pwm_slice_num, &config, false); // Push Configufatio
    pwm_set_chan_level(pedal_chorus_pwm_slice_num, pedal_chorus_pwm_channel, PEDAL_CHORUS_PWM_OFFSET); // Set Channel A
    pwm_set_chan_level(pedal_chorus_pwm_slice_num, pedal_chorus_pwm_channel + 1, PEDAL_CHORUS_PWM_OFFSET); // Set Channel B
    /* ADC Settings */
    adc_init();
    adc_gpio_init(PEDAL_CHORUS_ADC_0_GPIO); // GPIO26 (ADC0)
    adc_gpio_init(PEDAL_CHORUS_ADC_1_GPIO); // GPIO27 (ADC1)
    adc_gpio_init(PEDAL_CHORUS_ADC_2_GPIO); // GPIO28 (ADC2)
    adc_set_clkdiv(0.0f);
    adc_set_round_robin(0b00111);
    adc_fifo_setup(true, false, 3, true, false); // 12-bit Length (0-4095), Bit[15] for Error Flag
    adc_fifo_drain(); // Clean FIFO
    irq_set_exclusive_handler(ADC_IRQ_FIFO, pedal_chorus_on_adc_irq_fifo);
    irq_set_priority(ADC_IRQ_FIFO, 0xFF); // Highest Priority
    adc_irq_set_enabled(true);
    pedal_chorus_conversion_1 = PEDAL_CHORUS_ADC_MIDDLE_DEFAULT;
    pedal_chorus_conversion_2 = PEDAL_CHORUS_ADC_MIDDLE_DEFAULT;
    pedal_chorus_conversion_3 = PEDAL_CHORUS_ADC_MIDDLE_DEFAULT;
    pedal_chorus_conversion_1_temp = PEDAL_CHORUS_ADC_MIDDLE_DEFAULT;
    pedal_chorus_conversion_2_temp = PEDAL_CHORUS_ADC_MIDDLE_DEFAULT;
    pedal_chorus_conversion_3_temp = PEDAL_CHORUS_ADC_MIDDLE_DEFAULT;
    pedal_chorus_adc_middle_moving_average = pedal_chorus_conversion_1 * PEDAL_CHORUS_ADC_MIDDLE_NUMBER_MOVING_AVERAGE;
    pedal_chorus_osc_sine_1_time = 0;
    pedal_chorus_osc_sine_2_time = 0;
    pedal_chorus_osc_amplitude = PEDAL_CHORUS_OSC_AMPLITUDE_PEAK;
    pedal_chorus_osc_speed = 4;
    pedal_chorus_gain = pedal_chorus_conversion_2 >> 8; // Make 4-bit Value (0-15)
    pedal_chorus_noise_gate_threshold = (pedal_chorus_conversion_3 >> 8) * PEDAL_CHORUS_NOISE_GATE_THRESHOLD_MULTIPLIER; // Make 4-bit Value (0-15)
    pedal_chorus_noise_gate_count = 0;
    /* Start IRQ, PWM and ADC */
    irq_set_mask_enabled(0b1 << PWM_IRQ_WRAP|0b1 << ADC_IRQ_FIFO, true);
    pwm_set_mask_enabled(0b1 << pedal_chorus_pwm_slice_num);
    pedal_chorus_is_outstanding_on_adc = true;
    adc_select_input(0); // Ensure to Start from A0
    adc_run(true);
    while (true) {
        tight_loop_contents();
    }
}

void pedal_chorus_on_pwm_irq_wrap() {
    //uint32 from_time = time_us_32();
    uint16 conversion_1_temp = pedal_chorus_conversion_1_temp;
    uint16 conversion_2_temp = pedal_chorus_conversion_2_temp;
    uint16 conversion_3_temp = pedal_chorus_conversion_3_temp;
    if (! pedal_chorus_is_outstanding_on_adc) {
        pedal_chorus_is_outstanding_on_adc = true;
        adc_select_input(0); // Ensure to Start from A0
        adc_run(true); // Stable Starting Point after PWM IRQ
    }
    pedal_chorus_conversion_1 = conversion_1_temp;
    if (abs(conversion_2_temp - pedal_chorus_conversion_2) > PEDAL_CHORUS_ADC_THRESHOLD) {
        pedal_chorus_conversion_2 = conversion_2_temp;
        pedal_chorus_gain = pedal_chorus_conversion_2 >> 8; // Make 4-bit Value (0-15)
    }
    if (abs(conversion_3_temp - pedal_chorus_conversion_3) > PEDAL_CHORUS_ADC_THRESHOLD) {
        pedal_chorus_conversion_3 = conversion_3_temp;
        pedal_chorus_noise_gate_threshold = pedal_chorus_conversion_3 >> 8; // Make 4-bit Value (0-15)
        pedal_chorus_noise_gate_threshold *= PEDAL_CHORUS_NOISE_GATE_THRESHOLD_MULTIPLIER;
    }
    uint32 middle_moving_average = pedal_chorus_adc_middle_moving_average / PEDAL_CHORUS_ADC_MIDDLE_NUMBER_MOVING_AVERAGE;
    pedal_chorus_adc_middle_moving_average -= middle_moving_average;
    pedal_chorus_adc_middle_moving_average += pedal_chorus_conversion_1;
    int32 normalized_1 = pedal_chorus_conversion_1 - middle_moving_average;
    /**
     * pedal_chorus_noise_gate_count:
     *
     * Over Positive Threshold       ## 1
     *-----------------------------------------------------------------------------------------------------------
     * Under Positive Threshold     # 0 # 2      ### Reset to 1
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                  # 0   # 3   # 5   # 2
     *-----------------------------------------------------------------------------------------------------------
     * 0                           # 0   # 4   # 4   # 3   # 5 ...Count Up to PEDAL_CHORUS_NOISE_GATE_COUNT_MAX
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                         # 5 # 3      #### 4
     *-----------------------------------------------------------------------------------------------------------
     * Under Negative Threshold           # 6 # 2
     *-----------------------------------------------------------------------------------------------------------
     * Over Negative Threshold             ## Reset to 1
     */
    if (normalized_1 > pedal_chorus_noise_gate_threshold || normalized_1 < -pedal_chorus_noise_gate_threshold) {
        pedal_chorus_noise_gate_count = 1;
    } else if (pedal_chorus_noise_gate_count != 0 && (normalized_1 > (pedal_chorus_noise_gate_threshold >> 1) || normalized_1 < -(pedal_chorus_noise_gate_threshold >> 1))) {
        pedal_chorus_noise_gate_count = 1;
    } else if (pedal_chorus_noise_gate_count != 0) {
        pedal_chorus_noise_gate_count++;
    }
    if (pedal_chorus_noise_gate_count >= PEDAL_CHORUS_NOISE_GATE_COUNT_MAX) pedal_chorus_noise_gate_count = 0;
    if (pedal_chorus_noise_gate_count == 0) {
        normalized_1 = 0;
        pedal_chorus_osc_sine_1_time = 0;
        pedal_chorus_osc_sine_2_time = 0;
    }
    /**
     * Using 32-bit Signed Fixed Decimal, Bit[31:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    int32 fixed_point_value_sine_1 = pedal_chorus_table_sine_1[((uint32)pedal_chorus_osc_sine_1_time * (uint32)pedal_chorus_osc_speed) % PEDAL_CHORUS_OSC_SINE_1_TIME_MAX];
    int32 fixed_point_value_sine_2 = pedal_chorus_table_sine_2[((uint32)pedal_chorus_osc_sine_2_time * (uint32)pedal_chorus_osc_speed) % PEDAL_CHORUS_OSC_SINE_2_TIME_MAX] >> 1; // Divide By 2
    int32 osc_value = (int32)(int64)(((int64)(pedal_chorus_osc_amplitude << 16) * ((int64)fixed_point_value_sine_1 + (int64)fixed_point_value_sine_2)) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication
    //int32 osc_value = (int32)(int64)(((int64)(pedal_chorus_osc_amplitude << 16) * (int64)fixed_point_value_sine_1) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication
    osc_value = (int32)(int64)(((int64)(osc_value << 16) * (int64)(abs(normalized_1) << 4)) >> 32); // Absolute normalized_2 to Bit[15:0] Decimal Part
    normalized_1 += (int16)osc_value;
    pedal_chorus_osc_sine_1_time++;
    pedal_chorus_osc_sine_2_time++;
    if (pedal_chorus_osc_sine_1_time >= PEDAL_CHORUS_OSC_SINE_1_TIME_MAX) pedal_chorus_osc_sine_1_time = 0;
    if (pedal_chorus_osc_sine_2_time >= PEDAL_CHORUS_OSC_SINE_2_TIME_MAX) pedal_chorus_osc_sine_2_time = 0;
    if (pedal_chorus_gain > 7) {
        normalized_1 *= (pedal_chorus_gain - 7);
    } else {
        normalized_1 /= abs(pedal_chorus_gain - 8);
    }
    normalized_1 += middle_moving_average;
    if (normalized_1 > PEDAL_CHORUS_PWM_OFFSET + PEDAL_CHORUS_PWM_PEAK) {
        normalized_1 = PEDAL_CHORUS_PWM_OFFSET + PEDAL_CHORUS_PWM_PEAK;
    } else if (normalized_1 < PEDAL_CHORUS_PWM_OFFSET - PEDAL_CHORUS_PWM_PEAK) {
        normalized_1 = PEDAL_CHORUS_PWM_OFFSET - PEDAL_CHORUS_PWM_PEAK;
    }
    pwm_set_chan_level(pedal_chorus_pwm_slice_num, pedal_chorus_pwm_channel, (uint16)normalized_1);
    pwm_set_chan_level(pedal_chorus_pwm_slice_num, pedal_chorus_pwm_channel + 1, (uint16)normalized_1);
    pwm_clear_irq(pedal_chorus_pwm_slice_num); // Seems Overlap IRQ Otherwise
    //pedal_chorus_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_chorus_debug_time); // To send a made pointer, sync flag, etc.
}

void pedal_chorus_on_adc_irq_fifo() {
    adc_run(false);
    uint16 adc_fifo_level = adc_fifo_get_level(); // Seems 8 at Maximum
    //printf("@pedal_chorus_on_adc_irq_fifo 1 - adc_fifo_level: %d\n", adc_fifo_level);
    for (uint16 i = 0; i < adc_fifo_level; i++) {
        //printf("@pedal_chorus_on_adc_irq_fifo 2 - i: %d\n", i);
        uint16 temp = adc_fifo_get();
        temp &= 0x7FFF; // Clear Bit[15]: ERR
        uint16 remainder = i % 3;
        if (remainder == 2) {
            pedal_chorus_conversion_3_temp = temp;
        } else if (remainder == 1) {
            pedal_chorus_conversion_2_temp = temp;
        } else if (remainder == 0) {
            pedal_chorus_conversion_1_temp = temp;
        }
    }
    //printf("@pedal_chorus_on_adc_irq_fifo 3 - adc_fifo_is_empty(): %d\n", adc_fifo_is_empty());
    adc_fifo_drain();
    pedal_chorus_is_outstanding_on_adc = false;
}
