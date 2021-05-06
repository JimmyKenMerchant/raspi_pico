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
#include "util_pedal_pico.h"
#include "util_pedal_pico_ex.h"

#define PEDAL_DISTORTION_TRANSIENT_RESPONSE 100000 // 100000 Micro Seconds
#define PEDAL_DISTORTION_CORE_1_STACK_SIZE 1024 * 4 // 1024 Words, 4096 Bytes
#define PEDAL_DISTORTION_LED_GPIO 25

int main(void) {
    //stdio_init_all();
    util_pedal_pico_set_sys_clock_115200khz();
    //stdio_init_all(); // Re-init for UART Baud Rate
    sleep_us(PEDAL_DISTORTION_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(PEDAL_DISTORTION_LED_GPIO);
    gpio_set_dir(PEDAL_DISTORTION_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_DISTORTION_LED_GPIO, true);
    /* Initialize PWM and Switch */
    pedal_pico_distortion = util_pedal_pico_init(UTIL_PEDAL_PICO_PWM_1_GPIO, UTIL_PEDAL_PICO_PWM_2_GPIO);
    /* Initialize ADC */
    util_pedal_pico_init_adc();
    /* Assign Actual Array */
    #if UTIL_PEDAL_PICO_EX_PEAK == UTIL_PEDAL_PICO_PWM_PEAK
        pedal_pico_distortion_table_pdf_1 = util_pedal_pico_ex_table_pdf_1;
        pedal_pico_distortion_table_log_1 = util_pedal_pico_ex_table_log_1;
        pedal_pico_distortion_table_log_2 = util_pedal_pico_ex_table_log_2;
        pedal_pico_distortion_table_power_1 = util_pedal_pico_ex_table_power_1;
    #else
        #error Failure on Assigning Actual Array to pedal_pico_distortion_table_pdf_1
        #error Failure on Assigning Actual Array to pedal_pico_distortion_table_log_1
        #error Failure on Assigning Actual Array to pedal_pico_distortion_table_log_2
        #error Failure on Assigning Actual Array to pedal_pico_distortion_table_power_1
    #endif
    /* Unique Variables and Functions */
    pedal_pico_distortion_set();
    util_pedal_pico_on_pwm_irq_wrap_handler = (void*)util_pedal_pico_on_pwm_irq_wrap_handler_single;
    util_pedal_pico_process = pedal_pico_distortion_process;
    /* Launch Core 1 */
    uint32* stack_pointer = (int32*)malloc(PEDAL_DISTORTION_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(util_pedal_pico_start, stack_pointer, PEDAL_DISTORTION_CORE_1_STACK_SIZE);
    while (true) {
        //printf("@main 1 - pedal_pico_distortion_conversion_1 %0x\n", pedal_pico_distortion_conversion_1);
        //printf("@main 2 - pedal_pico_distortion_conversion_2 %0x\n", pedal_pico_distortion_conversion_2);
        //printf("@main 3 - pedal_pico_distortion_conversion_3 %0x\n", pedal_pico_distortion_conversion_3);
        //printf("@main 4 - multicore_fifo_pop_blocking() %d\n", multicore_fifo_pop_blocking());
        //printf("@main 5 - util_pedal_pico_debug_time %d\n", util_pedal_pico_debug_time);
        //sleep_ms(500);
        __wfi();
    }
    return 0;
}
