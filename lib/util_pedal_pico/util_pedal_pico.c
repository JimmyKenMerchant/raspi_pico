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

#include "util_pedal_pico.h"

void util_pedal_pico_init_adc() {
    /* ADC Settings */
    adc_init();
    adc_gpio_init(UTIL_PEDAL_PICO_ADC_0_GPIO); // GPIO26 (ADC0)
    adc_gpio_init(UTIL_PEDAL_PICO_ADC_1_GPIO); // GPIO27 (ADC1)
    adc_gpio_init(UTIL_PEDAL_PICO_ADC_2_GPIO); // GPIO28 (ADC2)
    adc_set_clkdiv(0.0f);
    adc_set_round_robin(0b00111);
    adc_fifo_setup(true, false, 3, true, false); // 12-bit Length (0-4095), Bit[15] for Error Flag
    adc_fifo_drain(); // Clean FIFO
    irq_set_exclusive_handler(ADC_IRQ_FIFO, util_pedal_pico_on_adc_irq_fifo);
    irq_set_priority(ADC_IRQ_FIFO, 0xFF); // Highest Priority
    adc_irq_set_enabled(true);
    util_pedal_pico_on_adc_is_outstanding = true;
    __dsb();
}

void util_pedal_pico_on_adc_irq_fifo() {
    adc_run(false);
    __dsb();
    __isb();
    uint16 adc_fifo_level = adc_fifo_get_level(); // Seems 8 at Maximum
    //printf("@util_pedal_pico_on_adc_irq_fifo 1 - adc_fifo_level: %d\n", adc_fifo_level);
    for (uint16 i = 0; i < adc_fifo_level; i++) {
        //printf("@util_pedal_pico_on_adc_irq_fifo 2 - i: %d\n", i);
        uint16 temp = adc_fifo_get();
        if (temp & 0x8000) { // Procedure on Malfunction
            reset_block(RESETS_RESET_PWM_BITS|RESETS_RESET_ADC_BITS);
            break;
        } else {
            temp &= 0x7FFF; // Clear Bit[15]: ERR
            uint16 remainder = i % 3;
            if (remainder == 2) {
                util_pedal_pico_on_adc_conversion_3 = temp;
            } else if (remainder == 1) {
                util_pedal_pico_on_adc_conversion_2 = temp;
            } else if (remainder == 0) {
                util_pedal_pico_on_adc_conversion_1 = temp;
            }
        }
    }
    //printf("@util_pedal_pico_on_adc_irq_fifo 3 - adc_fifo_is_empty(): %d\n", adc_fifo_is_empty());
    adc_fifo_drain();
    do {
        tight_loop_contents();
    } while (! adc_fifo_is_empty);
    util_pedal_pico_on_adc_is_outstanding = false;
    __dsb();
}


void util_pedal_pico_sw_loop(uchar8 gpio_1, uchar8 gpio_2) {
    uint32 gpio_mask = 0b1<< gpio_1|0b1 << gpio_2;
    gpio_init_mask(gpio_mask);
    gpio_set_dir_masked(gpio_mask, 0x00000000);
    gpio_pull_up(gpio_1);
    gpio_pull_up(gpio_2);
    util_pedal_pico_sw_mode = 0;
    uint16 count_sw_0 = 0; // Center
    uint16 count_sw_1 = 0;
    uint16 count_sw_2 = 0;
    uint32 state_sw_1 = 0b1 << gpio_2;
    uint32 state_sw_2 = 0b1 << gpio_1;
    uchar8 mode = 0; // To Reduce Memory Access
    while (true) {
        uint32 status_sw = gpio_get_all() & (0b1 << gpio_1|0b1 << gpio_2);
        if (status_sw == state_sw_1) {
            count_sw_0 = 0;
            count_sw_1++;
            count_sw_2 = 0;
            if (count_sw_1 >= UTIL_PEDAL_PICO_SW_THRESHOLD) {
                count_sw_1 = 0;
                if (mode != 1) {
                    util_pedal_pico_sw_mode = 1;
                    mode = 1;
                }
            }
        } else if (status_sw == state_sw_2) {
            count_sw_0 = 0;
            count_sw_1 = 0;
            count_sw_2++;
            if (count_sw_2 >= UTIL_PEDAL_PICO_SW_THRESHOLD) {
                count_sw_2 = 0;
                if (mode != 2) {
                    util_pedal_pico_sw_mode = 2;
                    mode = 2;
                }
            }
        } else {
            count_sw_0++;
            count_sw_1 = 0;
            count_sw_2 = 0;
            if (count_sw_0 >= UTIL_PEDAL_PICO_SW_THRESHOLD) {
                count_sw_0 = 0;
                if (mode != 0) {
                    util_pedal_pico_sw_mode = 0;
                    mode = 0;
                }
            }
        }
        sleep_us(UTIL_PEDAL_PICO_SW_SLEEP_TIME);
        __dsb();
    }
}
