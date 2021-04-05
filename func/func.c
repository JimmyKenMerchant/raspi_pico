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
// Dependancies
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
// raspi_pico/include
#include "macros_pico.h"
#include "function_generator_pico.h"

#define FUNC_PWM_1_GPIO 16
#define FUNC_PWM_OFFSET 2048
#define FUNC_PWM_PEAK 2047

function_generator_pico* function_generator;

uint32 func_pwm_slice_num;
uint32 func_pwm_channel;
uint32 func_next_factor;
uint32 func_debug_time;

void func_on_pwm_irq_wrap();

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // Wait for Rediness of USB for Messages
    /* PWM Settings */
    gpio_set_function(FUNC_PWM_1_GPIO, GPIO_FUNC_PWM); // GPIO16 = PWM8 A
    func_pwm_slice_num = pwm_gpio_to_slice_num(FUNC_PWM_1_GPIO); // GPIO16 = PWM8
    func_pwm_channel = pwm_gpio_to_channel(FUNC_PWM_1_GPIO); // GPIO16 = A
    // Set IRQ and Handler for PWM
    pwm_clear_irq(func_pwm_slice_num);
    pwm_set_irq_enabled(func_pwm_slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, func_on_pwm_irq_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);
    // PWM Configuration (Make Approx. 30518Hz from 125Mhz - 0.032768ms Cycle)
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    pwm_config_set_clkdiv(&config, 1.0f); // Set Clock Divider, 125,000,000 Divided by 1.0 for 0.008us Cycle
    pwm_config_set_wrap(&config, 4095); // 0-4095, 4096 Cycles for 0.032768ms
    pwm_init(func_pwm_slice_num, &config, false); // Push Configufatio
    pwm_set_chan_level(func_pwm_slice_num, func_pwm_channel, FUNC_PWM_OFFSET);
    func_next_factor = 0;
    function_generator = function_generator_pico_init(25, func_next_factor, FUNC_PWM_PEAK); // 30518 divided by 25 = 1220.72Hz
    /* Start PWM */
    pwm_set_mask_enabled(0b1 << func_pwm_slice_num);
    printf("@main 1 - Let's Start!\n");
    while (true) {
        puts("Type 0-5 to Change Frequency:");
        printf("@main 2 - func_debug_time %d\n", func_debug_time);
        int32 input = getchar_timeout_us(10000000);
        if (input == PICO_ERROR_TIMEOUT) continue;
        if (input >= 0x30 && input <= 0x35) { // "0-5" to 0-5
            printf("%c\n", input);
            input -= 0x30;
            func_next_factor = input;
        } else {
            continue;
        }
    }
    free(function_generator);
    return 0;
}

void func_on_pwm_irq_wrap() {
    uint32 from_time = time_us_32();
    pwm_set_chan_level(func_pwm_slice_num, func_pwm_channel, function_generator_pico_sine(function_generator) + FUNC_PWM_OFFSET);
    if(function_generator->is_end) {
        function_generator->factor = func_next_factor;
        function_generator->is_end = false;
    }
    func_debug_time = time_us_32() - from_time;
    pwm_clear_irq(func_pwm_slice_num);
}
