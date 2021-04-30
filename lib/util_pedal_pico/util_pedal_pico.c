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

void util_pedal_pico_set_sys_clock_115200khz() {
    /**
     * XOSC_MHZ is in platform_defs.h of pico-sdk
     * MHZ and KHZ are in clocks.h of pico-sdk
     */
    if (XOSC_MHZ * MHZ == UTIL_PEDAL_PICO_XOSC) {
        set_sys_clock_pll(XOSC_MHZ * MHZ * 96, 5, 2);
    } else {
        panic("Failure on Changing System Clock in Function, \"util_pedal_pico_set_clock_115200khz\"");
    }
}

void util_pedal_pico_set_pwm_28125hz(pwm_config* ptr_config) {
    /* Divide 115200Khz by 4096hz */
    pwm_config_set_clkdiv(ptr_config, 1.0f); // Set Clock Divider, 115,200,000 Divided by 1.0
    pwm_config_set_wrap(ptr_config, 4095); // 0-4095, 4096 Cycles
}

util_pedal_pico* util_pedal_pico_init(uchar8 gpio_1, uchar8 gpio_2) {
    /* PWM Settings */
    gpio_set_function(gpio_1, GPIO_FUNC_PWM);
    gpio_set_function(gpio_2, GPIO_FUNC_PWM);
    util_pedal_pico* util_pedal = (util_pedal_pico*)malloc(sizeof(util_pedal_pico));
    util_pedal->pwm_1_slice = pwm_gpio_to_slice_num(gpio_1);
    util_pedal->pwm_1_channel = pwm_gpio_to_channel(gpio_1);
    util_pedal->pwm_2_slice = pwm_gpio_to_slice_num(gpio_2);
    util_pedal->pwm_2_channel = pwm_gpio_to_channel(gpio_2);
    // Set IRQ and Handler for PWM
    pwm_clear_irq(util_pedal->pwm_1_slice);
    pwm_set_irq_enabled(util_pedal->pwm_1_slice, true);
    // PWM Configuration
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    util_pedal_pico_set_pwm_28125hz(&config);
    pwm_init(util_pedal->pwm_1_slice, &config, false); // Push Configration
    if (util_pedal->pwm_1_slice != util_pedal->pwm_2_slice) pwm_init(util_pedal->pwm_2_slice, &config, false); // Push Configration
    util_pedal_pico_sw_mode = 0; // Initialize Mode of Switch Before Running PWM and ADC
    __dsb();
    return util_pedal;
}

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
    util_pedal_pico_on_adc_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    util_pedal_pico_on_adc_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    util_pedal_pico_on_adc_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    util_pedal_pico_adc_middle_moving_average = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    util_pedal_pico_adc_middle_moving_average_sum = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT * UTIL_PEDAL_PICO_ADC_MIDDLE_MOVING_AVERAGE_NUMBER;
    __dsb();
}

void util_pedal_pico_start(util_pedal_pico* util_pedal) {
    /* Start IRQ, PWM and ADC */
    util_pedal_pico_on_adc_is_outstanding = true;
    irq_set_mask_enabled(0b1 << PWM_IRQ_WRAP|0b1 << ADC_IRQ_FIFO, true);
    pwm_set_mask_enabled(0b1 << util_pedal->pwm_1_slice|0b1 << util_pedal->pwm_2_slice);
    adc_select_input(0); // Ensure to Start from A0
    __dsb();
    __isb();
    adc_run(true);
}

void util_pedal_pico_stop(util_pedal_pico* util_pedal) {
    pwm_clear_irq(util_pedal->pwm_1_slice);
    adc_run(false);
    __dsb();
    irq_set_mask_enabled(0b1 << PWM_IRQ_WRAP|0b1 << ADC_IRQ_FIFO, false);
    pwm_set_mask_enabled(0);
    adc_fifo_drain(); // Clean FIFO
    do {
        tight_loop_contents();
    } while (! adc_fifo_is_empty);
    __dsb();
}

void util_pedal_pico_renew_adc_middle_moving_average(uint16 conversion) {
    uint32 middle_moving_average = util_pedal_pico_adc_middle_moving_average_sum / UTIL_PEDAL_PICO_ADC_MIDDLE_MOVING_AVERAGE_NUMBER;
    util_pedal_pico_adc_middle_moving_average_sum -= middle_moving_average;
    util_pedal_pico_adc_middle_moving_average_sum += conversion;
    util_pedal_pico_adc_middle_moving_average = (uint16)middle_moving_average;
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
            if (time_us_64() >= UTIL_PEDAL_PICO_ADC_ERROR_SINCE) {
                reset_block(RESETS_RESET_PWM_BITS|RESETS_RESET_ADC_BITS);
                panic("Detected ADC Error in Function, \"util_pedal_pico_on_adc_irq_fifo\"");
            }
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
