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
#include "pedal_pico/pedal_pico_sideband.h"
#include "util_pedal_pico.h"
#include "util_pedal_pico_ex.h"

#define PEDAL_SIDEBAND_TRANSIENT_RESPONSE 100000 // 100000 Micro Seconds
#define PEDAL_SIDEBAND_CORE_1_STACK_SIZE 1024 * 4 // 1024 Words, 4096 Bytes
#define PEDAL_SIDEBAND_LED_GPIO 25

int main(void) {
    util_pedal_pico_set_sys_clock_115200khz();
    sleep_us(PEDAL_SIDEBAND_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(PEDAL_SIDEBAND_LED_GPIO);
    gpio_set_dir(PEDAL_SIDEBAND_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_SIDEBAND_LED_GPIO, 1);
    /* Initialize PWM and Switch */
    pedal_pico_sideband = util_pedal_pico_init(UTIL_PEDAL_PICO_PWM_1_GPIO, UTIL_PEDAL_PICO_PWM_2_GPIO);
    /* Initialize ADC */
    util_pedal_pico_init_adc();
    /* Assign Actual Array */
    #if UTIL_PEDAL_PICO_EX_PEAK == UTIL_PEDAL_PICO_PWM_PEAK
        pedal_pico_sideband_table_pdf_1 = util_pedal_pico_ex_table_pdf_1;
        pedal_pico_sideband_table_pdf_2 = util_pedal_pico_ex_table_pdf_2;
        pedal_pico_sideband_table_pdf_3 = util_pedal_pico_ex_table_pdf_3;
    #else
        #error Failure on Assigning Actual Array to pedal_pico_sideband_table_pdf_1
        #error Failure on Assigning Actual Array to pedal_pico_sideband_table_pdf_2
        #error Failure on Assigning Actual Array to pedal_pico_sideband_table_pdf_3
    #endif
    #if UTIL_PEDAL_PICO_EX_OSC_TIME_MAX == PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MAX
        pedal_pico_sideband_table_sine_1 = util_pedal_pico_ex_table_sine_1;
    #else
        #error Failure on Assigning Actual Array to pedal_pico_sideband_table_sine_1
    #endif
    /* Initialize Switch */
    util_pedal_pico_init_sw(UTIL_PEDAL_PICO_SW_1_GPIO, UTIL_PEDAL_PICO_SW_2_GPIO);
    /* Unique Variables and Functions */
    pedal_pico_sideband_set();
    util_pedal_pico_on_pwm_irq_wrap_handler = (void*)util_pedal_pico_on_pwm_irq_wrap_handler_single;
    util_pedal_pico_process = pedal_pico_sideband_process;
    /* Launch Core 1 */
    uint32* stack_pointer = (int32*)malloc(PEDAL_SIDEBAND_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(util_pedal_pico_start, stack_pointer, PEDAL_SIDEBAND_CORE_1_STACK_SIZE);
    while (true) {
        util_pedal_pico_wait_loop();
    }
    return 0;
}
