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
#include "sequencer_pwm_pico.h"

#define SERVO_PWM_1_GPIO 2
#define SERVO_ADC_0_GPIO 26
#define SERVO_PWM_THRESHOLD 0xF

uint16 servo_sequence[] = {0x8000|720,0x8000|720,0x8000|800,0x8000|880,0x8000|960,0x8000|1040,0x8000|1120,0x8000|1200,0x8000|1200,0x8000|1280,0x8000|1360,0x8000|1440,0x8000|1520,0x8000|1600,0x8000|1680,0x8000|1680,0x0000}; // Clear MSB to Show End of Sequence, 900-2100us (120 degrees)

sequencer_pwm_pico* servo_the_sequencer_1;

bool servo_is_outstanding_on_adc;
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
    // Set IRQ and Handler for PWM
    pwm_clear_irq(servo_pwm_slice_num);
    pwm_set_irq_enabled(servo_pwm_slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, servo_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0x80); // Middle Priority
    irq_set_enabled(PWM_IRQ_WRAP, true);
    // PWM Configuration (Make 50Hz from 125Mhz - 12.5ms Cycle)
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    pwm_config_set_clkdiv(&config, 156.25f); // Set Clock Divider, 125,000,000 Divided by 156.25 for 1.25us Cycle
    pwm_config_set_wrap(&config, 9999); // 0-9999, 10,000 Cycles for 12.5ms
    pwm_init(servo_pwm_slice_num, &config, false); // Push Configufation
    pwm_set_chan_level(servo_pwm_slice_num, servo_pwm_channel, servo_sequence[7] & 0x7FFF); // Assuming Channel A, 1500us
    // PWM Sequence Settings
    servo_the_sequencer_1 = sequencer_pwm_pico_init((SERVO_PWM_1_GPIO % 2) << 7|(SERVO_PWM_1_GPIO / 2), servo_sequence);
    printf("@main 1 - servo_the_sequencer_1->sequence_length: %d\n", servo_the_sequencer_1->sequence_length);
    /* ADC Settings */
    adc_init();
    adc_gpio_init(SERVO_ADC_0_GPIO); // GPIO26 (ADC0) for GPIO2 (PWM1 A)
    adc_set_clkdiv(0.0f);
    adc_set_round_robin(0b0001);
    adc_fifo_setup(true, false, 1, true, true); // Truncate to 8-bit Length (0-255)
    adc_fifo_drain(); // Clean FIFO
    irq_set_exclusive_handler(ADC_IRQ_FIFO, servo_on_adc_irq_fifo);
    irq_set_priority(ADC_IRQ_FIFO, 0xFF); // Highest Priority
    adc_irq_set_enabled(true);
    servo_conversion_1 = 112; // >> 4 Makes 7
    servo_conversion_1_temp = 0;
    /* Start IRQ, PWM and ADC */
    irq_set_mask_enabled(0b1 << PWM_IRQ_WRAP|0b1 << ADC_IRQ_FIFO, true);
    pwm_set_mask_enabled(0b1 << servo_pwm_slice_num);
    servo_is_outstanding_on_adc = true;
    adc_select_input(0); // Ensure to Start from A0
    adc_run(true);
    printf("@main 2 - Let's Start!\n");
    while (true) {
        tight_loop_contents();
    }
    return 0;
}

void servo_on_pwm_irq_wrap() {
    pwm_clear_irq(servo_pwm_slice_num);
    if (abs(servo_conversion_1_temp - servo_conversion_1) > SERVO_PWM_THRESHOLD) {
        servo_conversion_1 = servo_conversion_1_temp;
        servo_the_sequencer_1->index = servo_conversion_1 >> 4;
        sequencer_pwm_pico_execute(servo_the_sequencer_1);
    }
    //pwm_set_chan_level(servo_pwm_slice_num, servo_pwm_channel, (servo_conversion_1 >> 4) << 4);
    //printf("@servo_on_pwm_irq_wrap 1 - servo_conversion_1: %d\n", servo_conversion_1);
    if (! servo_is_outstanding_on_adc) {
        servo_is_outstanding_on_adc = true;
        adc_select_input(0); // Ensure to Start from A0
        adc_run(true);
    }
}

void servo_on_adc_irq_fifo() {
    adc_run(false);
    uint16 adc_fifo_level = adc_fifo_get_level();
    //printf("@servo_on_adc_irq_fifo 1 - adc_fifo_level: %d\n", adc_fifo_level);
    for (uint16 i = 0; i < adc_fifo_level; i++) {
        //printf("@servo_on_adc_irq_fifo 2 - i: %d\n", i);
        servo_conversion_1_temp = adc_fifo_get();
    }
    //printf("@servo_on_adc_irq_fifo 3 - adc_fifo_is_empty(): %d\n", adc_fifo_is_empty());
    adc_fifo_drain();
    servo_is_outstanding_on_adc = false;
}
