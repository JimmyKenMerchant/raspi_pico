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
// raspi_pico/include
#include "macros_pico.h"
//#include "function_generator_pico.h"

#define PEDAL_BUFFER_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_BUFFER_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_BUFFER_PWM_OFFSET 2048
#define PEDAL_BUFFER_PWM_PEAK 2047
#define PEDAL_BUFFER_NOISE_GATE_THRESHOLD_MULTIPLIER 1 // From -66db (2048) to -42db (128)
#define PEDAL_BUFFER_ADC_0_GPIO 26
#define PEDAL_BUFFER_ADC_1_GPIO 27
#define PEDAL_BUFFER_ADC_2_GPIO 28
#define PEDAL_BUFFER_ADC_MIDDLE 2048
#define PEDAL_BUFFER_ADC_THRESHOLD 0x7F // Range is 0x0-0xFFF (0-4095) Divided by 0xFF (255) for 0x0-0xFb (0-15). 0xFF >> 1.

//function_generator_pico* function_generator;

uint32 pedal_buffer_pwm_slice_num;
uint32 pedal_buffer_pwm_channel;
uint16 pedal_buffer_conversion_1;
uint16 pedal_buffer_conversion_2;
uint16 pedal_buffer_conversion_3;
uint16 pedal_buffer_conversion_1_temp;
uint16 pedal_buffer_conversion_2_temp;
uint16 pedal_buffer_conversion_3_temp;
char8 pedal_buffer_gain;
char8 pedal_buffer_noise_gate_threshold;
bool pedal_buffer_is_outstanding_on_adc;

void pedal_buffer_core_1();
void pedal_buffer_on_pwm_irq_wrap();
void pedal_buffer_on_adc_irq_fifo();

int main(void) {
    //stdio_init_all();
    //sleep_ms(2000); // Wait for Rediness of USB for Messages
    multicore_launch_core1(pedal_buffer_core_1);
    //printf("@main 1 - Let's Start!\n");
    while (true) {
        //printf("@main 2 - pedal_buffer_conversion_1 %0x\n", pedal_buffer_conversion_1);
        //printf("@main 3 - pedal_buffer_conversion_2 %0x\n", pedal_buffer_conversion_2);
        //printf("@main 4 - pedal_buffer_conversion_3 %0x\n", pedal_buffer_conversion_3);
        //printf("@main 5 - debug_time %d\n", multicore_fifo_pop_blocking());
        //sleep_ms(500);
        tight_loop_contents();
    }
    return 0;
}

void pedal_buffer_core_1() {
    /* PWM Settings */
    gpio_set_function(PEDAL_BUFFER_PWM_1_GPIO, GPIO_FUNC_PWM); // GPIO16 = PWM8 A
    gpio_set_function(PEDAL_BUFFER_PWM_2_GPIO, GPIO_FUNC_PWM); // GPIO17 = PWM8 B
    pedal_buffer_pwm_slice_num = pwm_gpio_to_slice_num(PEDAL_BUFFER_PWM_1_GPIO); // GPIO16 = PWM8
    pedal_buffer_pwm_channel = pwm_gpio_to_channel(PEDAL_BUFFER_PWM_1_GPIO); // GPIO16 = A
    // Set IRQ and Handler for PWM
    pwm_clear_irq(pedal_buffer_pwm_slice_num);
    pwm_set_irq_enabled(pedal_buffer_pwm_slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_buffer_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0x80); // Middle Priority
    // PWM Configuration (Make Approx. 30518Hz from 125Mhz - 0.032768ms Cycle)
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    pwm_config_set_clkdiv(&config, 1.0f); // Set Clock Divider, 125,000,000 Divided by 1.0 for 0.008us Cycle
    pwm_config_set_wrap(&config, 4095); // 0-4095, 4096 Cycles for 0.032768ms
    pwm_init(pedal_buffer_pwm_slice_num, &config, false); // Push Configufatio
    pwm_set_chan_level(pedal_buffer_pwm_slice_num, pedal_buffer_pwm_channel, PEDAL_BUFFER_PWM_OFFSET); // Set Channel A
    pwm_set_chan_level(pedal_buffer_pwm_slice_num, pedal_buffer_pwm_channel + 1, PEDAL_BUFFER_PWM_OFFSET); // Set Channel B
    /* ADC Settings */
    adc_init();
    adc_gpio_init(PEDAL_BUFFER_ADC_0_GPIO); // GPIO26 (ADC0)
    adc_gpio_init(PEDAL_BUFFER_ADC_1_GPIO); // GPIO27 (ADC1)
    adc_gpio_init(PEDAL_BUFFER_ADC_2_GPIO); // GPIO28 (ADC2)
    adc_set_clkdiv(0.0f);
    adc_set_round_robin(0b0111);
    adc_fifo_setup(true, false, 3, true, false); // 12-bit Length (0-4095), Bit[15] for Error Flag
    adc_fifo_drain(); // Clean FIFO
    irq_set_exclusive_handler(ADC_IRQ_FIFO, pedal_buffer_on_adc_irq_fifo);
    irq_set_priority(ADC_IRQ_FIFO, 0xFF); // Highest Priority
    adc_irq_set_enabled(true);
    pedal_buffer_conversion_1 = PEDAL_BUFFER_ADC_MIDDLE;
    pedal_buffer_conversion_2 = PEDAL_BUFFER_ADC_MIDDLE;
    pedal_buffer_conversion_3 = PEDAL_BUFFER_ADC_MIDDLE;
    pedal_buffer_conversion_1_temp = PEDAL_BUFFER_ADC_MIDDLE;
    pedal_buffer_conversion_2_temp = PEDAL_BUFFER_ADC_MIDDLE;
    pedal_buffer_conversion_3_temp = PEDAL_BUFFER_ADC_MIDDLE;
    pedal_buffer_gain = PEDAL_BUFFER_ADC_MIDDLE >> 8; // Make 4-bit Value (0-15)
    pedal_buffer_noise_gate_threshold =  PEDAL_BUFFER_ADC_MIDDLE >> 8; // Make 4-bit Value (0-15)
    pedal_buffer_noise_gate_threshold *= PEDAL_BUFFER_NOISE_GATE_THRESHOLD_MULTIPLIER;
    /* Start IRQ, PWM and ADC */
    irq_set_mask_enabled(0b1 << PWM_IRQ_WRAP|0b1 << ADC_IRQ_FIFO, true);
    pwm_set_mask_enabled(0b1 << pedal_buffer_pwm_slice_num);
    pedal_buffer_is_outstanding_on_adc = true;
    adc_select_input(0); // Ensure to Start from A0
    adc_run(true);
    while (true) {
        tight_loop_contents();
    }
}

void pedal_buffer_on_pwm_irq_wrap() {
    //absolute_time_t from_time = get_absolute_time();
    pedal_buffer_conversion_1 = pedal_buffer_conversion_1_temp;
    if (abs(pedal_buffer_conversion_2_temp - pedal_buffer_conversion_2) > PEDAL_BUFFER_ADC_THRESHOLD) {
        pedal_buffer_conversion_2 = pedal_buffer_conversion_2_temp;
        pedal_buffer_gain = pedal_buffer_conversion_2 >> 8; // Make 4-bit Value (0-15)
    }
    if (abs(pedal_buffer_conversion_3_temp - pedal_buffer_conversion_3) > PEDAL_BUFFER_ADC_THRESHOLD) {
        pedal_buffer_conversion_3 = pedal_buffer_conversion_3_temp;
        pedal_buffer_noise_gate_threshold = pedal_buffer_conversion_3 >> 8; // Make 4-bit Value (0-15)
        pedal_buffer_noise_gate_threshold *= PEDAL_BUFFER_NOISE_GATE_THRESHOLD_MULTIPLIER;
    }
    int32 normalized_1 = pedal_buffer_conversion_1 - PEDAL_BUFFER_PWM_OFFSET;
    if (abs(normalized_1) < pedal_buffer_noise_gate_threshold) normalized_1 = 0;
    if (pedal_buffer_gain > 7) {
        normalized_1 *= (pedal_buffer_gain - 7);
    } else {
        normalized_1 /= abs(pedal_buffer_gain - 8);
    }
    if (normalized_1 > PEDAL_BUFFER_PWM_PEAK) {
        normalized_1 = PEDAL_BUFFER_PWM_PEAK;
    } else if (normalized_1 < -PEDAL_BUFFER_PWM_PEAK) {
        normalized_1 = -PEDAL_BUFFER_PWM_PEAK;
    }
    pwm_set_chan_level(pedal_buffer_pwm_slice_num, pedal_buffer_pwm_channel, (uint16)(normalized_1 + PEDAL_BUFFER_PWM_OFFSET));
    pwm_set_chan_level(pedal_buffer_pwm_slice_num, pedal_buffer_pwm_channel + 1, (uint16)(normalized_1 + PEDAL_BUFFER_PWM_OFFSET));
    //multicore_fifo_push_blocking(absolute_time_diff_us(from_time, get_absolute_time())); // To send a made pointer, sync flag, etc.
    if (! pedal_buffer_is_outstanding_on_adc) {
        pedal_buffer_is_outstanding_on_adc = true;
        adc_select_input(0); // Ensure to Start from A0
        adc_run(true);
    }
    pwm_clear_irq(pedal_buffer_pwm_slice_num); // Seems Overwarp IRQ Otherwise
}

void pedal_buffer_on_adc_irq_fifo() {
    adc_run(false);
    uint16 adc_fifo_level = adc_fifo_get_level(); // Seems 8 at Maximum
    //printf("@pedal_buffer_on_adc_irq_fifo 1 - adc_fifo_level: %d\n", adc_fifo_level);
    for (uint16 i = 0; i < adc_fifo_level; i++) {
        //printf("@pedal_buffer_on_adc_irq_fifo 2 - i: %d\n", i);
        uint16 temp = adc_fifo_get();
        temp &= 0x7FFF; // Clear Bit[15]: ERR
        uint16 remainder = i % 3;
        if (remainder == 2) {
            pedal_buffer_conversion_3_temp = temp;
        } else if (remainder == 1) {
            pedal_buffer_conversion_2_temp = temp;
        } else if (remainder == 0) {
            pedal_buffer_conversion_1_temp = temp;
        }
    }
    //printf("@pedal_buffer_on_adc_irq_fifo 3 - adc_fifo_is_empty(): %d\n", adc_fifo_is_empty());
    adc_fifo_drain();
    pedal_buffer_is_outstanding_on_adc = false;
}
