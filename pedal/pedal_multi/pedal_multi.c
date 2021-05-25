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
#include "pedal_pico/pedal_pico_buffer.h"
#include "pedal_pico/pedal_pico_sideband.h"
#include "pedal_pico/pedal_pico_chorus.h"
#include "pedal_pico/pedal_pico_reverb.h"
#include "pedal_pico/pedal_pico_tape.h"
#include "pedal_pico/pedal_pico_phaser.h"
#include "pedal_pico/pedal_pico_planets.h"
#include "pedal_pico/pedal_pico_distortion.h"
#include "util_pedal_pico.h"
#include "util_pedal_pico_ex.h"

#define PEDAL_PICO_MULTI_SLEEP_TIME 100000 // 100000 Micro Seconds

int main(void) {
    util_pedal_pico_set_sys_clock_115200khz();
    sleep_us(UTIL_PEDAL_PICO_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(UTIL_PEDAL_PICO_LED_1_GPIO);
    gpio_set_dir(UTIL_PEDAL_PICO_LED_1_GPIO, GPIO_OUT);
    gpio_put(UTIL_PEDAL_PICO_LED_1_GPIO, 1);
    /* Initialize PWM and Switch */
    pedal_pico_buffer = util_pedal_pico_init(UTIL_PEDAL_PICO_PWM_1_GPIO, UTIL_PEDAL_PICO_PWM_2_GPIO);
    pedal_pico_sideband = util_pedal_pico_obj;
    pedal_pico_chorus = util_pedal_pico_obj;
    pedal_pico_reverb = util_pedal_pico_obj;
    pedal_pico_tape = util_pedal_pico_obj;
    pedal_pico_phaser = util_pedal_pico_obj;
    pedal_pico_planets = util_pedal_pico_obj;
    pedal_pico_distortion = util_pedal_pico_obj;
    /* Initialize ADC */
    util_pedal_pico_init_adc();
    /* Initialize Multi Functions */
    util_pedal_pico_init_multi(UTIL_PEDAL_PICO_MULTI_BIT_0_GPIO, UTIL_PEDAL_PICO_MULTI_BIT_1_GPIO, UTIL_PEDAL_PICO_MULTI_BIT_2_GPIO);
    /* Set Individual Functions to Multi Functions */
    util_pedal_pico_multi_set[0] = pedal_pico_buffer_set;
    util_pedal_pico_multi_set[1] = pedal_pico_sideband_set;
    util_pedal_pico_multi_set[2] = pedal_pico_chorus_set;
    util_pedal_pico_multi_set[3] = pedal_pico_reverb_set;
    util_pedal_pico_multi_set[4] = pedal_pico_tape_set;
    util_pedal_pico_multi_set[5] = pedal_pico_phaser_set;
    util_pedal_pico_multi_set[6] = pedal_pico_planets_set;
    util_pedal_pico_multi_set[7] = pedal_pico_distortion_set;
    util_pedal_pico_multi_process[0] = pedal_pico_buffer_process;
    util_pedal_pico_multi_process[1] = pedal_pico_sideband_process;
    util_pedal_pico_multi_process[2] = pedal_pico_chorus_process;
    util_pedal_pico_multi_process[3] = pedal_pico_reverb_process;
    util_pedal_pico_multi_process[4] = pedal_pico_tape_process;
    util_pedal_pico_multi_process[5] = pedal_pico_phaser_process;
    util_pedal_pico_multi_process[6] = pedal_pico_planets_process;
    util_pedal_pico_multi_process[7] = pedal_pico_distortion_process;
    util_pedal_pico_multi_free[0] = pedal_pico_buffer_free;
    util_pedal_pico_multi_free[1] = pedal_pico_sideband_free;
    util_pedal_pico_multi_free[2] = pedal_pico_chorus_free;
    util_pedal_pico_multi_free[3] = pedal_pico_reverb_free;
    util_pedal_pico_multi_free[4] = pedal_pico_tape_free;
    util_pedal_pico_multi_free[5] = pedal_pico_phaser_free;
    util_pedal_pico_multi_free[6] = pedal_pico_planets_free;
    util_pedal_pico_multi_free[7] = pedal_pico_distortion_free;
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
    util_pedal_pico_select_multi();
    /* Launch Core 1 */
    uint32* stack_pointer = (int32*)malloc(UTIL_PEDAL_PICO_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(util_pedal_pico_start, stack_pointer, UTIL_PEDAL_PICO_CORE_1_STACK_SIZE);
    while (true) {
        util_pedal_pico_select_multi();
        sleep_us(PEDAL_PICO_MULTI_SLEEP_TIME);
    }
    return 0;
}
