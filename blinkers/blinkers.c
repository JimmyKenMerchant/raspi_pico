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
#include "sequencer_gpio_pico.h"

uchar8 blinkers_pinlist[] = {15,16,25}; // Pin No. 0 to No. 29
uint32 blinkers_pinlist_length = 3;
uint16 blinkers_sequence[] = {0b1000000000000010,
                              0b1000000000000101,
                              0b1000000000000010,
                              0b1000000000000101,
                              0b1000000000000010,
                              0b1000000000000101,
                              0b0000000000000000, // Clear MSB to Show End of Sequence
                              };

sequencer_gpio_pico* blinkers_the_sequencer;

uint32 blinkers_count;
uint32 blinkers_pwm_slice_num;
uint32 blinkers_pwm_channel;
void blinkers_on_pwm_irq_wrap();

#define BLINKERS_PWM_GPIO 14

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // Wait for Rediness of USB for Messages
    /* PWM Settings */
    gpio_set_function(BLINKERS_PWM_GPIO, GPIO_FUNC_PWM); // GPIO14 = PWM7 A
    blinkers_pwm_slice_num = pwm_gpio_to_slice_num(BLINKERS_PWM_GPIO); // GPIO 14 = PWM7
    blinkers_pwm_channel = pwm_gpio_to_channel(BLINKERS_PWM_GPIO); // GPIO 14 = A
    printf("main 1 - blinkers_pwm_slice_num: %d\n", blinkers_pwm_slice_num);
    printf("main 2 - blinkers_pwm_channel: %d\n", blinkers_pwm_channel);
    // Set IRQ and Handler for PWM
    pwm_clear_irq(blinkers_pwm_slice_num);
    pwm_set_irq_enabled(blinkers_pwm_slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, blinkers_on_pwm_irq_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);
    // PWM Configuration
    pwm_set_wrap(blinkers_pwm_slice_num, 0xFFFF); // 0-65535, 65536 Cycles
    blinkers_count = 255;
    pwm_set_chan_level(blinkers_pwm_slice_num, blinkers_pwm_channel, blinkers_count * blinkers_count); // 0-0x7FFFF for High
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    pwm_config_set_clkdiv(&config, 5.5f); // Set Clock Divider
    pwm_init(blinkers_pwm_slice_num, &config, true); // Push Configufation and Start
    /* GPIO Settings */
    blinkers_the_sequencer = sequencer_gpio_pico_init(blinkers_pinlist, blinkers_pinlist_length, blinkers_sequence);
    printf("Let's Start!\n");
    while (true) {
        tight_loop_contents();
        //printf("In The Loop: %d\n", blinkers_the_sequencer->index);
        //sleep_ms(500);
    }
    free(blinkers_the_sequencer);
    return 0;
}

void blinkers_on_pwm_irq_wrap() {
    pwm_clear_irq(blinkers_pwm_slice_num);
    pwm_set_chan_level(blinkers_pwm_slice_num, blinkers_pwm_channel, blinkers_count * blinkers_count);
    blinkers_count--;
    if (blinkers_count == 0) {
        blinkers_count = 255;
        sequencer_gpio_pico_execute(blinkers_the_sequencer);
        printf("In The IRQ: %d\n", blinkers_the_sequencer->index);
    }
}
