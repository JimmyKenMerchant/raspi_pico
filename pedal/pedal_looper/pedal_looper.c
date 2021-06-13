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
#include "pedal_pico/pedal_pico_looper.h"
#include "util_pedal_pico_ex.h"

volatile uint32 pedal_looper_debug_time;

int main(void) {
    util_pedal_pico_set_sys_clock_115200khz();
    sleep_us(UTIL_PEDAL_PICO_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(UTIL_PEDAL_PICO_LED_1_GPIO);
    gpio_set_dir(UTIL_PEDAL_PICO_LED_1_GPIO, GPIO_OUT);
    gpio_put(UTIL_PEDAL_PICO_LED_1_GPIO, 1);
    /* Initialize PWM */
    #if UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX != UTIL_PEDAL_PICO_EX_OSC_SINE_TIME_MAX
        #error "UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX isn't eqaul to UTIL_PEDAL_PICO_EX_OSC_SINE_TIME_MAX. Include util_pedal_pico_ex.h?"
    #endif
    #if UTIL_PEDAL_PICO_OSC_TRIANGLE_1_TIME_MAX != UTIL_PEDAL_PICO_EX_OSC_TRIANGLE_TIME_MAX
        #error "UTIL_PEDAL_PICO_OSC_TRIANGLE_1_TIME_MAX isn't eqaul to UTIL_PEDAL_PICO_EX_OSC_TRIANGLE_TIME_MAX. Include util_pedal_pico_ex.h?"
    #endif
    #if UTIL_PEDAL_PICO_PWM_PEAK != UTIL_PEDAL_PICO_EX_PEAK
        #error "UTIL_PEDAL_PICO_PWM_PEAK isn't eqaul to UTIL_PEDAL_PICO_EX_PEAK. Include util_pedal_pico_ex.h?"
    #endif
    pedal_pico_looper = util_pedal_pico_init(UTIL_PEDAL_PICO_PWM_1_GPIO, UTIL_PEDAL_PICO_PWM_2_GPIO);
    /* Initialize ADC */
    util_pedal_pico_init_adc();
    /* Initialize Switch */
    util_pedal_pico_init_sw(UTIL_PEDAL_PICO_SW_1_GPIO, UTIL_PEDAL_PICO_SW_2_GPIO);
    /* Unique Variables and Functions */
    pedal_pico_looper_set(UTIL_PEDAL_PICO_LED_2_GPIO);
    util_pedal_pico_process = pedal_pico_looper_process;
    /* Launch Core 1 */
    uint32* stack_pointer = (int32*)malloc(UTIL_PEDAL_PICO_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(util_pedal_pico_start, stack_pointer, UTIL_PEDAL_PICO_CORE_1_STACK_SIZE);
    #if UTIL_PEDAL_PICO_DEBUG
        pedal_looper_debug_time = 0;
    #endif
    while (true) {
        #if UTIL_PEDAL_PICO_DEBUG
            uint32 from_time = time_us_32();
        #endif
        pedal_pico_looper_flash_handler();
        #if UTIL_PEDAL_PICO_DEBUG
            pedal_looper_debug_time = time_us_32() - from_time;
            printf("@main 1 - pedal_looper_debug_time %d\n", pedal_looper_debug_time);
            //util_pedal_pico_wait(); // Causes Delay on Recording
        #endif
    }
    return 0;
}
