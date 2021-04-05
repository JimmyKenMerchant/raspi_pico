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

#define TWIN_DIMMERS_PWM_1_GPIO 14
#define TWIN_DIMMERS_PWM_2_GPIO 15
#define TWIN_DIMMERS_ADC_0_GPIO 26
#define TWIN_DIMMERS_ADC_1_GPIO 27
#define TWIN_DIMMERS_PWM_THRESHOLD (255/16) >> 2 // Maximum Value (0xFF = 255) of ADC Divide by Number of Array of twin_dimmers_sequence (16), Expecting 15
#define TWIN_DIMMERS_COUNT_MAX 32

uint16 twin_dimmers_sequence[] = {0x8000,0x8020,0x8030,0x8040,0x8050,0x8060,0x8070,0x8080,0x8090,0x80A0,0x80B0,0x80C0,0x80D0,0x80E0,0x80F0,0x8100,0x0000}; // 0 (No Pulse) to 256 (High on Full Range), Clear MSB to Show End of Sequence

sequencer_pwm_pico* twin_dimmers_the_sequencer_1;
sequencer_pwm_pico* twin_dimmers_the_sequencer_2;

bool twin_dimmers_is_outstanding_on_adc;
uint32 twin_dimmers_count;
uint32 twin_dimmers_pwm_slice_num;
uint32 twin_dimmers_pwm_channel;
uint16 twin_dimmers_conversion_1;
uint16 twin_dimmers_conversion_2;
uint16 twin_dimmers_conversion_1_temp;
uint16 twin_dimmers_conversion_2_temp;
void twin_dimmers_on_pwm_irq_wrap();
void twin_dimmers_on_adc_irq_fifo();

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // Wait for Rediness of USB for Messages
    /* PWM Settings */
    gpio_set_function(TWIN_DIMMERS_PWM_1_GPIO, GPIO_FUNC_PWM); // GPIO14 = PWM7 A
    gpio_set_function(TWIN_DIMMERS_PWM_2_GPIO, GPIO_FUNC_PWM); // GPIO15 = PWM7 B
    twin_dimmers_pwm_slice_num = pwm_gpio_to_slice_num(TWIN_DIMMERS_PWM_1_GPIO); // GPIO14 = PWM7
    twin_dimmers_pwm_channel = pwm_gpio_to_channel(TWIN_DIMMERS_PWM_1_GPIO); // GPIO14 = A
    // Set IRQ and Handler for PWM
    pwm_clear_irq(twin_dimmers_pwm_slice_num);
    pwm_set_irq_enabled(twin_dimmers_pwm_slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, twin_dimmers_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0x80); // Middle Priority
    irq_set_enabled(PWM_IRQ_WRAP, true);
    // PWM Configuration (Make Arrox. 975Hz from 125Mhz - Approx. 1ms Cycle)
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    pwm_config_set_clkdiv(&config, 500.75f); // Set Clock Divider, 125,000,000 Divided by 500.75 for Approx. 4us for A Cycle
    pwm_config_set_wrap(&config, 0xFF); // 0-255, 256 Cycles for Approx. 1ms
    pwm_init(twin_dimmers_pwm_slice_num, &config, false); // Push Configufation
    twin_dimmers_count = TWIN_DIMMERS_COUNT_MAX;
    pwm_set_chan_level(twin_dimmers_pwm_slice_num, twin_dimmers_pwm_channel, 0); // Assuming Channel A
    pwm_set_chan_level(twin_dimmers_pwm_slice_num, twin_dimmers_pwm_channel + 1, 0); // Assuming Channel B
    // PWM Sequence Settings
    twin_dimmers_the_sequencer_1 = sequencer_pwm_pico_init((TWIN_DIMMERS_PWM_1_GPIO % 2) << 7|(TWIN_DIMMERS_PWM_1_GPIO / 2), twin_dimmers_sequence);
    twin_dimmers_the_sequencer_2 = sequencer_pwm_pico_init((TWIN_DIMMERS_PWM_2_GPIO % 2) << 7|(TWIN_DIMMERS_PWM_2_GPIO / 2), twin_dimmers_sequence);
    printf("@main 1 - twin_dimmers_the_sequencer_1->sequence_length: %d\n", twin_dimmers_the_sequencer_1->sequence_length);
    /* ADC Settings */
    adc_init();
    adc_gpio_init(TWIN_DIMMERS_ADC_0_GPIO); // GPIO26 (ADC0) for GPIO14 (PWM7 A)
    adc_gpio_init(TWIN_DIMMERS_ADC_1_GPIO); // GPIO27 (ADC1) for GPIO15 (PWM7 B)
    adc_set_clkdiv(0.0f);
    adc_set_round_robin(0b00011);
    adc_fifo_setup(true, false, 2, true, true); // Truncate to 8-bit Length (0-255)
    adc_fifo_drain(); // Clean FIFO
    irq_set_exclusive_handler(ADC_IRQ_FIFO, twin_dimmers_on_adc_irq_fifo);
    irq_set_priority(ADC_IRQ_FIFO, 0xFF); // Highest Priority
    adc_irq_set_enabled(true);
    twin_dimmers_conversion_1 = 0;
    twin_dimmers_conversion_2 = 0;
    twin_dimmers_conversion_1_temp = 0;
    twin_dimmers_conversion_2_temp = 0;
    /* Start IRQ, PWM and ADC */
    irq_set_mask_enabled(0b1 << PWM_IRQ_WRAP|0b1 << ADC_IRQ_FIFO, true);
    pwm_set_mask_enabled(0b1 << twin_dimmers_pwm_slice_num);
    twin_dimmers_is_outstanding_on_adc = true;
    adc_select_input(0); // Ensure to Start from A0
    adc_run(true);
    printf("@main 2 - Let's Start!\n");
    while (true) {
        tight_loop_contents();
    }
    return 0;
}

void twin_dimmers_on_pwm_irq_wrap() {
    twin_dimmers_count--;
    if (twin_dimmers_count == 0) {
        if (abs(twin_dimmers_conversion_1_temp - twin_dimmers_conversion_1) > TWIN_DIMMERS_PWM_THRESHOLD) {
            twin_dimmers_conversion_1 = twin_dimmers_conversion_1_temp;
            twin_dimmers_the_sequencer_1->index = twin_dimmers_conversion_1 >> 4;
            sequencer_pwm_pico_execute(twin_dimmers_the_sequencer_1);
        }
        if (abs(twin_dimmers_conversion_2_temp - twin_dimmers_conversion_2) > TWIN_DIMMERS_PWM_THRESHOLD) {
            twin_dimmers_conversion_2 = twin_dimmers_conversion_2_temp;
            twin_dimmers_the_sequencer_2->index = twin_dimmers_conversion_2 >> 4;
            sequencer_pwm_pico_execute(twin_dimmers_the_sequencer_2);
        }
        //pwm_set_chan_level(twin_dimmers_pwm_slice_num, twin_dimmers_pwm_channel, (twin_dimmers_conversion_1 >> 4) << 4);
        //pwm_set_chan_level(twin_dimmers_pwm_slice_num, twin_dimmers_pwm_channel + 1, (twin_dimmers_conversion_2 >> 4) << 4);
        twin_dimmers_count = TWIN_DIMMERS_COUNT_MAX;
        //printf("@twin_dimmers_on_pwm_irq_wrap 1 - twin_dimmers_conversion_1: %d, twin_dimmers_conversion_2: %d\n", twin_dimmers_conversion_1, twin_dimmers_conversion_2);
    }
    if (! twin_dimmers_is_outstanding_on_adc) {
        twin_dimmers_is_outstanding_on_adc = true;
        adc_select_input(0); // Ensure to Start from A0
        adc_run(true);
    }
    pwm_clear_irq(twin_dimmers_pwm_slice_num);
}

void twin_dimmers_on_adc_irq_fifo() {
    adc_run(false);
    uint16 adc_fifo_level = adc_fifo_get_level();
    //printf("@twin_dimmers_on_adc_irq_fifo 1 - adc_fifo_level: %d\n", adc_fifo_level);
    for (uint16 i = 0; i < adc_fifo_level; i++) {
        //printf("@twin_dimmers_on_adc_irq_fifo 2 - i: %d\n", i);
        uint16 temp = adc_fifo_get();
        temp &= 0x7FFF; // Clear Bit[15]: ERR
        if (i % 2) {
            twin_dimmers_conversion_2_temp = temp;
        } else {
            twin_dimmers_conversion_1_temp = temp;
        }
    }
    //printf("@twin_dimmers_on_adc_irq_fifo 3 - adc_fifo_is_empty(): %d\n", adc_fifo_is_empty());
    adc_fifo_drain();
    twin_dimmers_is_outstanding_on_adc = false;
}
