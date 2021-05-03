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
#include "pedal_pico/pedal_pico_tape.h"
#include "util_pedal_pico.h"
#include "util_pedal_pico_ex.h"

#define PEDAL_TAPE_TRANSIENT_RESPONSE 100000 // 100000 Micro Seconds
#define PEDAL_TAPE_CORE_1_STACK_SIZE 1024 * 4 // 1024 Words, 4096 Bytes
#define PEDAL_TAPE_LED_GPIO 25

int main(void) {
    //stdio_init_all();
    util_pedal_pico_set_sys_clock_115200khz();
    //stdio_init_all(); // Re-init for UART Baud Rate
    sleep_us(PEDAL_TAPE_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(PEDAL_TAPE_LED_GPIO);
    gpio_set_dir(PEDAL_TAPE_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_TAPE_LED_GPIO, true);
    /* Initialize PWM */
    pedal_pico_tape = util_pedal_pico_init(PEDAL_PICO_TAPE_PWM_1_GPIO, PEDAL_PICO_TAPE_PWM_2_GPIO);
    /* Initialize ADC */
    util_pedal_pico_init_adc();
    /* Assign Actual Array */
    #if UTIL_PEDAL_PICO_EX_PEAK == PEDAL_PICO_TAPE_PWM_PEAK
        pedal_pico_tape_table_pdf_1 = util_pedal_pico_ex_table_pdf_1;
    #else
        #error Failure on Assigning Actual Array to pedal_pico_tape_table_pdf_1
    #endif
    #if UTIL_PEDAL_PICO_EX_OSC_TIME_MAX == PEDAL_PICO_TAPE_OSC_SINE_1_TIME_MAX
        pedal_pico_tape_table_sine_1 = util_pedal_pico_ex_table_sine_1;
    #else
        #error Failure on Assigning Actual Array to pedal_pico_tape_table_sine_1
    #endif
    uint32* stack_pointer = (int32*)malloc(PEDAL_TAPE_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(pedal_pico_tape_start, stack_pointer, PEDAL_TAPE_CORE_1_STACK_SIZE);
    //pedal_pico_tape_debug_time = 0;
    //uint32 from_time = time_us_32();
    //printf("@main 1 - Let's Start!\n");
    //pedal_pico_tape_debug_time = time_us_32() - from_time;
    //printf("@main 2 - pedal_pico_tape_debug_time %d\n", pedal_pico_tape_debug_time);
    while (true) {
        //printf("@main 3 - pedal_pico_tape_conversion_1 %0x\n", pedal_pico_tape_conversion_1);
        //printf("@main 4 - pedal_pico_tape_conversion_2 %0x\n", pedal_pico_tape_conversion_2);
        //printf("@main 5 - pedal_pico_tape_conversion_3 %0x\n", pedal_pico_tape_conversion_3);
        //printf("@main 6 - multicore_fifo_pop_blocking() %d\n", multicore_fifo_pop_blocking());
        //printf("@main 7 - pedal_pico_tape_debug_time %d\n", pedal_pico_tape_debug_time);
        //sleep_ms(500);
        //tight_loop_contents();
        __wfi();
    }
    return 0;
}
