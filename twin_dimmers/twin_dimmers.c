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
#include "hardware/adc.h"
#include "hardware/irq.h"
// raspi_pico/include
#include "macros_pico.h"

uint32 twin_dimmers_count;
uint32 twin_dimmers_pwm_slice_num;
uint32 twin_dimmers_pwm_channel;
uint16 twin_dimmers_conversion_1;
uint16 twin_dimmers_conversion_2;
void twin_dimmers_on_pwm_irq_wrap();
void twin_dimmers_on_adc_irq_fifo();

#define TWIN_DIMMERS_PWM_GPIO 14
#define TWIN_DIMMERS_COUNT_MAX 32

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // Wait for Rediness of USB for Messages
    /* PWM Settings */
    gpio_set_function(TWIN_DIMMERS_PWM_GPIO, GPIO_FUNC_PWM); // GPIO14 = PWM7 A
    gpio_set_function(TWIN_DIMMERS_PWM_GPIO + 1, GPIO_FUNC_PWM); // GPIO15 = PWM7 B
    twin_dimmers_pwm_slice_num = pwm_gpio_to_slice_num(TWIN_DIMMERS_PWM_GPIO); // GPIO 14 = PWM7
    twin_dimmers_pwm_channel = pwm_gpio_to_channel(TWIN_DIMMERS_PWM_GPIO); // GPIO 14 = A
    // Set IRQ and Handler for PWM
    pwm_clear_irq(twin_dimmers_pwm_slice_num);
    pwm_set_irq_enabled(twin_dimmers_pwm_slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, twin_dimmers_on_pwm_irq_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);
    // PWM Configuration
    pwm_set_wrap(twin_dimmers_pwm_slice_num, 0xFF); // 0-255, 65536 Cycles
    twin_dimmers_count = TWIN_DIMMERS_COUNT_MAX;
    pwm_set_chan_level(twin_dimmers_pwm_slice_num, twin_dimmers_pwm_channel, 0); // Assuming Channel A
    pwm_set_chan_level(twin_dimmers_pwm_slice_num, twin_dimmers_pwm_channel + 1, 0); // Assuming Channel B
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    pwm_config_set_clkdiv(&config, 255.75f); // Set Clock Divider
    pwm_init(twin_dimmers_pwm_slice_num, &config, false); // Push Configufation
    /* ADC Settings */
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_set_clkdiv(60000.0f);
    adc_set_round_robin(0b0011);
    adc_fifo_setup(true, false, 2, true, true); // Truncate to 8-bit Length (0-255)
    adc_fifo_drain(); // Clean FIFO
    adc_irq_set_enabled(true);
    irq_set_exclusive_handler(ADC_IRQ_FIFO, twin_dimmers_on_adc_irq_fifo);
    /* Start IRQ, PWM and ADC */
    irq_set_mask_enabled(0b1 << PWM_IRQ_WRAP|0b1 << ADC_IRQ_FIFO, true);
    pwm_set_mask_enabled(0b1 << twin_dimmers_pwm_slice_num);
    adc_run(true);
    printf("Let's Start!\n");
    while (true) {
        tight_loop_contents();
        //printf("In The Loop: %d\n", twin_dimmers_the_sequencer->index);
        //sleep_ms(500);
    }
    return 0;
}

void twin_dimmers_on_pwm_irq_wrap() {
    pwm_clear_irq(twin_dimmers_pwm_slice_num);
    pwm_set_chan_level(twin_dimmers_pwm_slice_num, twin_dimmers_pwm_channel, twin_dimmers_conversion_1);
    pwm_set_chan_level(twin_dimmers_pwm_slice_num, twin_dimmers_pwm_channel + 1, twin_dimmers_conversion_2);
    twin_dimmers_count--;
    if (twin_dimmers_count == 0) {
        twin_dimmers_count = TWIN_DIMMERS_COUNT_MAX;
        printf("In PWM IRQ - twin_dimmers_conversion_1: %d, twin_dimmers_conversion_2: %d\n", twin_dimmers_conversion_1, twin_dimmers_conversion_2);
    }
}

void twin_dimmers_on_adc_irq_fifo() {
    adc_run(false);
    uint16 adc_fifo_level = adc_fifo_get_level();
    //printf("In ADC IRQ: %d\n", adc_fifo_level);
    for (uint16 i = 0; i < adc_fifo_level; i++) {
        if (i % 2) {
            twin_dimmers_conversion_2 = adc_fifo_get();
        } else {
            twin_dimmers_conversion_1 = adc_fifo_get();
        }
        //printf("In ADC IRQ - conversion: %d\n", conversion);
    }
    adc_run(true);
}
