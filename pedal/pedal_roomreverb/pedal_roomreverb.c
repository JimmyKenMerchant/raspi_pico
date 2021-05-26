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
#include "pedal_pico/pedal_pico_reverb.h"
#include "pedal_pico/pedal_pico_chorus.h"
#include "util_pedal_pico_ex.h"

#define PEDAL_ROOMREVERB_REVERB_CONVERSION_2_FIXED_1 0
#define PEDAL_ROOMREVERB_REVERB_CONVERSION_2_FIXED_2 0x3FF
#define PEDAL_ROOMREVERB_REVERB_CONVERSION_2_FIXED_3 0x7FF
#define PEDAL_ROOMREVERB_CHORUS_CONVERSION_3_FIXED_1 0
#define PEDAL_ROOMREVERB_CHORUS_CONVERSION_3_FIXED_2 0x7FF
#define PEDAL_ROOMREVERB_CHORUS_CONVERSION_3_FIXED_3 0xFFF

uint16 pedal_roomreverb_reverb_conversion_2;
uint16 pedal_roomreverb_chorus_conversion_3;

void pedal_roomreverb_set();
void pedal_roomreverb_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode);

int main(void) {
    util_pedal_pico_set_sys_clock_115200khz();
    sleep_us(UTIL_PEDAL_PICO_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(UTIL_PEDAL_PICO_LED_1_GPIO);
    gpio_set_dir(UTIL_PEDAL_PICO_LED_1_GPIO, GPIO_OUT);
    gpio_put(UTIL_PEDAL_PICO_LED_1_GPIO, 1);
    /* Initialize PWM */
    #if UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX != UTIL_PEDAL_PICO_EX_OSC_TIME_MAX
        #error "UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX isn't eqaul to UTIL_PEDAL_PICO_EX_OSC_TIME_MAX. Include util_pedal_pico_ex.h?"
    #endif
    #if UTIL_PEDAL_PICO_PWM_PEAK != UTIL_PEDAL_PICO_EX_PEAK
        #error "UTIL_PEDAL_PICO_PWM_PEAK isn't eqaul to UTIL_PEDAL_PICO_EX_PEAK. Include util_pedal_pico_ex.h?"
    #endif
    pedal_pico_reverb = util_pedal_pico_init(UTIL_PEDAL_PICO_PWM_1_GPIO, UTIL_PEDAL_PICO_PWM_2_GPIO);
    pedal_pico_chorus = util_pedal_pico_obj;
    /* Initialize ADC */
    util_pedal_pico_init_adc();
    /* Initialize Switch */
    util_pedal_pico_init_sw(UTIL_PEDAL_PICO_SW_1_GPIO, UTIL_PEDAL_PICO_SW_2_GPIO);
    /* Unique Variables and Functions */
    pedal_roomreverb_set();
    util_pedal_pico_process = pedal_roomreverb_process;
    /* Launch Core 1 */
    uint32* stack_pointer = (int32*)malloc(UTIL_PEDAL_PICO_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(util_pedal_pico_start, stack_pointer, UTIL_PEDAL_PICO_CORE_1_STACK_SIZE);
    while (true) {
        util_pedal_pico_wait();
    }
    return 0;
}

void pedal_roomreverb_set() {
    pedal_pico_reverb_set();
    pedal_pico_chorus_set();
}

void pedal_roomreverb_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode) {
    if (sw_mode == 1) {
        pedal_roomreverb_reverb_conversion_2 = PEDAL_ROOMREVERB_REVERB_CONVERSION_2_FIXED_1;
        pedal_roomreverb_chorus_conversion_3 = PEDAL_ROOMREVERB_CHORUS_CONVERSION_3_FIXED_1;
    } else if (sw_mode == 2) {
        pedal_roomreverb_reverb_conversion_2 = PEDAL_ROOMREVERB_REVERB_CONVERSION_2_FIXED_3;
        pedal_roomreverb_chorus_conversion_3 = PEDAL_ROOMREVERB_CHORUS_CONVERSION_3_FIXED_3;
    } else {
        pedal_roomreverb_reverb_conversion_2 = PEDAL_ROOMREVERB_REVERB_CONVERSION_2_FIXED_2;
        pedal_roomreverb_chorus_conversion_3 = PEDAL_ROOMREVERB_CHORUS_CONVERSION_3_FIXED_2;
    }
    /* Objective entities, util_pedal_pico_obj, pedal_pico_reverb, and pedal_pico_chorus points the same struct and memory space */
    pedal_pico_reverb_process(conversion_1, pedal_roomreverb_reverb_conversion_2, conversion_2, 0);
    pedal_pico_chorus_process(util_pedal_pico_obj->output_1, conversion_3, pedal_roomreverb_chorus_conversion_3, 0);
}
