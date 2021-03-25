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

uint32 twin_dimmers_count;
uint32 twin_dimmers_pwm_slice_num;
uint32 twin_dimmers_pwm_channel;
void twin_dimmers_on_pwm_irq_wrap();

#define TWIN_DIMMERS_PWM_GPIO 14
#define TWIN_DIMMERS_COUNT_MAX 255

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
    pwm_set_wrap(twin_dimmers_pwm_slice_num, 0xFFFF); // 0-65535, 65536 Cycles
    twin_dimmers_count = TWIN_DIMMERS_COUNT_MAX;
    pwm_set_chan_level(twin_dimmers_pwm_slice_num, twin_dimmers_pwm_channel, TWIN_DIMMERS_COUNT_MAX * TWIN_DIMMERS_COUNT_MAX); // Assuming Channel A
    pwm_set_chan_level(twin_dimmers_pwm_slice_num, twin_dimmers_pwm_channel + 1, TWIN_DIMMERS_COUNT_MAX * TWIN_DIMMERS_COUNT_MAX); // Assuming Channel B
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    pwm_config_set_clkdiv(&config, 5.5f); // Set Clock Divider
    pwm_init(twin_dimmers_pwm_slice_num, &config, true); // Push Configufation and Start
    /* GPIO Settings */
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
    pwm_set_chan_level(twin_dimmers_pwm_slice_num, twin_dimmers_pwm_channel, twin_dimmers_count * twin_dimmers_count);
    pwm_set_chan_level(twin_dimmers_pwm_slice_num, twin_dimmers_pwm_channel + 1, TWIN_DIMMERS_COUNT_MAX * TWIN_DIMMERS_COUNT_MAX - (twin_dimmers_count * twin_dimmers_count));
    twin_dimmers_count--;
    if (twin_dimmers_count == 0) {
        twin_dimmers_count = TWIN_DIMMERS_COUNT_MAX;
        printf("In The IRQ: %d\n", twin_dimmers_count);
    }
}
