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
// raspi_pico/include
#include "macros_pico.h"
#include "sequencer_pwm_pico.h"

#define SERVO_PWM_1_GPIO 2

uint16 servo_sequence[] = {0x8000|720,0x8000|720,0x8000|800,0x8000|880,0x8000|960,0x8000|1040,0x8000|1120,0x8000|1200,0x8000|1200,0x8000|1280,0x8000|1360,0x8000|1440,0x8000|1520,0x8000|1600,0x8000|1680,0x8000|1680,0x0000}; // Clear MSB to Show End of Sequence, 900-2100us (120 degrees)

sequencer_pwm_pico* servo_the_sequencer_1;

uint32 servo_pwm_slice_num;
uint32 servo_pwm_channel;
uint16 servo_conversion_1;
uint16 servo_conversion_1_temp;
void servo_on_pwm_irq_wrap();
void servo_on_adc_irq_fifo();

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // Wait for Rediness of USB for Messages
    /* PWM Settings */
    gpio_set_function(SERVO_PWM_1_GPIO, GPIO_FUNC_PWM); // GPIO2 = PWM1 A
    servo_pwm_slice_num = pwm_gpio_to_slice_num(SERVO_PWM_1_GPIO); // GPIO2 = PWM1
    servo_pwm_channel = pwm_gpio_to_channel(SERVO_PWM_1_GPIO); // GPIO2 = A
    // PWM Configuration (Make 50Hz from 125Mhz - 12.5ms Cycle)
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    pwm_config_set_clkdiv(&config, 156.25f); // Set Clock Divider, 125,000,000 Divided by 156.25 for 1.25us Cycle
    pwm_config_set_wrap(&config, 9999); // 0-9999, 10,000 Cycles for 12.5ms
    pwm_init(servo_pwm_slice_num, &config, false); // Push Configufatio
    pwm_set_chan_level(servo_pwm_slice_num, servo_pwm_channel, servo_sequence[7] & 0x7FFF); // Assuming Channel A, 1500us
    // PWM Sequence Settings
    servo_the_sequencer_1 = sequencer_pwm_pico_init((SERVO_PWM_1_GPIO % 2) << 7|(SERVO_PWM_1_GPIO / 2), servo_sequence);
    /* Start PWM */
    pwm_set_mask_enabled(0b1 << servo_pwm_slice_num);
    printf("@main 2 - Let's Start!\n");
    while (true) {
        puts("Type 0-F to Change Position of Servo Motor:");
        int32 input = getchar_timeout_us(10000000);
        if (input == PICO_ERROR_TIMEOUT) continue;
        printf("%c\n", input);
        if (input >= 0x30 && input <= 0x39) { // "0-9" to 0-9
            input -= 0x30;
        } else if (input >= 0x41 && input <= 0x46) { // "A-F" to 10-15
            input -= 0x37; // 55
        } else if (input >= 0x61 && input <= 0x66) { // "a-f" to 10-15
            input -= 0x57; // 87
        } else {
            continue;
        }
        servo_the_sequencer_1->index = input;
        sequencer_pwm_pico_execute(servo_the_sequencer_1);
        //tight_loop_contents();
    }
    return 0;
}
