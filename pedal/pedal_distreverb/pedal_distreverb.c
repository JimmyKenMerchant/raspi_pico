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
// raspi_pico/include
#include "macros_pico.h"
#include "pedal_pico/pedal_pico_distortion.h"
#include "pedal_pico/pedal_pico_reverb.h"
#include "util_pedal_pico.h"
#include "util_pedal_pico_ex.h"

#define PEDAL_DISTREVERB_DISTORTION_CONVERSION_2_FIXED_1 0xFFF

uint16 pedal_distreverb_reverb_conversion_2;

void pedal_distreverb_set();
void pedal_distreverb_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode);

int main(void) {
    util_pedal_pico_set_sys_clock_115200khz();
    sleep_us(UTIL_PEDAL_PICO_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(UTIL_PEDAL_PICO_LED_1_GPIO);
    gpio_set_dir(UTIL_PEDAL_PICO_LED_1_GPIO, GPIO_OUT);
    gpio_put(UTIL_PEDAL_PICO_LED_1_GPIO, 1);
    /* Initialize PWM and Switch */
    pedal_pico_distortion = util_pedal_pico_init(UTIL_PEDAL_PICO_PWM_1_GPIO, UTIL_PEDAL_PICO_PWM_2_GPIO);
    pedal_pico_reverb = util_pedal_pico_obj;
    /* Initialize ADC */
    util_pedal_pico_init_adc();
    /* Assign Actual Array */
    #if UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX == UTIL_PEDAL_PICO_EX_OSC_TIME_MAX
        util_pedal_pico_table_sine_1 = util_pedal_pico_ex_table_sine_1;
    #else
        #error "Failure on Assigning Actual Array to util_pedal_pico_table_sine_1"
    #endif
    #if UTIL_PEDAL_PICO_PWM_PEAK == UTIL_PEDAL_PICO_EX_PEAK
        util_pedal_pico_table_pdf_1 = util_pedal_pico_ex_table_pdf_1;
        util_pedal_pico_table_pdf_2 = util_pedal_pico_ex_table_pdf_2;
        util_pedal_pico_table_pdf_3 = util_pedal_pico_ex_table_pdf_3;
        util_pedal_pico_table_log_1 = util_pedal_pico_ex_table_log_1;
        util_pedal_pico_table_log_2 = util_pedal_pico_ex_table_log_2;
        util_pedal_pico_table_power_1 = util_pedal_pico_ex_table_power_1;
    #else
        #error "Failure on Assigning Actual Array to util_pedal_pico_table_pdf_1"
        #error "Failure on Assigning Actual Array to util_pedal_pico_table_pdf_2"
        #error "Failure on Assigning Actual Array to util_pedal_pico_table_pdf_3"
        #error "Failure on Assigning Actual Array to util_pedal_pico_table_log_1"
        #error "Failure on Assigning Actual Array to util_pedal_pico_table_log_2"
        #error "Failure on Assigning Actual Array to util_pedal_pico_table_power_1"
    #endif
    /* Initialize Switch */
    util_pedal_pico_init_sw(UTIL_PEDAL_PICO_SW_1_GPIO, UTIL_PEDAL_PICO_SW_2_GPIO);
    /* Unique Variables and Functions */
    pedal_distreverb_set();
    util_pedal_pico_process = pedal_distreverb_process;
    /* Launch Core 1 */
    uint32* stack_pointer = (int32*)malloc(UTIL_PEDAL_PICO_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(util_pedal_pico_start, stack_pointer, UTIL_PEDAL_PICO_CORE_1_STACK_SIZE);
    while (true) {
        util_pedal_pico_wait();
    }
    return 0;
}

void pedal_distreverb_set() {
    pedal_pico_distortion_set();
    pedal_pico_reverb_set();
}

void pedal_distreverb_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    /* Objective entities, util_pedal_pico_obj, pedal_pico_distortion, and pedal_pico_reverb points the same struct and memory space */
    pedal_pico_distortion_process(conversion_1, PEDAL_DISTREVERB_DISTORTION_CONVERSION_2_FIXED_1, 0, sw_mode);
    pedal_pico_reverb_process(util_pedal_pico_obj->output_1, conversion_2, conversion_3, 0);
}
