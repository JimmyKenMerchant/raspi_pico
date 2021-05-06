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

#ifndef _UTIL_PEDAL_PICO_H
#define _UTIL_PEDAL_PICO_H 1

// Standards
#include <stdio.h>
#include <stdlib.h>
// Dependancies
#include "pico/stdlib.h"
#include "pico/divider.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/resets.h"
#include "hardware/clocks.h"
// raspi_pico/include
#include "macros_pico.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions */

#define UTIL_PEDAL_PICO_XOSC 12000000 // Assuming Crystal Clock Speed
#define UTIL_PEDAL_PICO_SW_1_GPIO 14
#define UTIL_PEDAL_PICO_SW_2_GPIO 15
#define UTIL_PEDAL_PICO_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define UTIL_PEDAL_PICO_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define UTIL_PEDAL_PICO_PWM_OFFSET 2048 // Ideal Middle Point
#define UTIL_PEDAL_PICO_PWM_PEAK 2047
#define UTIL_PEDAL_PICO_ADC_0_GPIO 26
#define UTIL_PEDAL_PICO_ADC_1_GPIO 27
#define UTIL_PEDAL_PICO_ADC_2_GPIO 28
#define UTIL_PEDAL_PICO_ADC_ERROR_SINCE (uint64)1000000 // System Time (Micro Seconds) to Start Handling ADC Error
#define UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT 2048
#define UTIL_PEDAL_PICO_ADC_MIDDLE_MOVING_AVERAGE_NUMBER 16384 // Should be Power of 2 Because of Processing Speed (Logical Shift Left on Division)
#define UTIL_PEDAL_PICO_ADC_THRESHOLD 0x3F // Range is 0x0-0xFFF (0-4095) Divided by 0x80 (128) for 0x0-0x1F (0-31), (0x80 >> 1) - 1.

#define UTIL_PEDAL_PICO_SW_THRESHOLD 30
#define UTIL_PEDAL_PICO_SW_SLEEP_TIME 1000
#define util_pedal_pico_cutoff_normalized(x, y) (_max(-y, _min(x, y))) // x: Value, y: Absolute Peak
#define util_pedal_pico_cutoff_biased(x, y, z) (_max(z, _min(x, y))) // x: Value, y: Peak, z: Bottom
#define util_pedal_pico_interpolate(x, y, z) ((x) == (y) ? (x) : ((x) > (y) ? (x - z) : (x + z))) // x: Base, y: Purpose, z: Value to Accumulate

/* Structs */

typedef struct {
    uchar8 pwm_1_slice;
    uchar8 pwm_1_channel;
    uchar8 pwm_2_slice;
    uchar8 pwm_2_channel;
    int32 output_1;
    int32 output_1_inverted;
} util_pedal_pico;

/* Global Variables */

volatile util_pedal_pico* util_pedal_pico_obj; // Pointer Needed to Be Initialized
volatile uint16 util_pedal_pico_on_adc_conversion_1;
volatile uint16 util_pedal_pico_on_adc_conversion_2;
volatile uint16 util_pedal_pico_on_adc_conversion_3;
volatile bool util_pedal_pico_on_adc_is_outstanding;
volatile uint16 util_pedal_pico_adc_middle_moving_average;
volatile uint32 util_pedal_pico_adc_middle_moving_average_sum;
volatile uchar8 util_pedal_pico_sw_1_gpio;
volatile uchar8 util_pedal_pico_sw_2_gpio;
volatile uchar8 util_pedal_pico_sw_mode;
volatile uint32 util_pedal_pico_debug_time;

/* Functions */

void (*util_pedal_pico_on_pwm_irq_wrap_handler)(); // Pointer Needed to Be Assigned
void (*util_pedal_pico_process)(uint16, uint16, uint16, uchar8); // Pointer Needed to Be Assigned
void util_pedal_pico_set_sys_clock_115200khz();
void util_pedal_pico_set_pwm_28125hz(pwm_config* ptr_config);
util_pedal_pico* util_pedal_pico_init(uchar8 gpio_1, uchar8 gpio_2);
void util_pedal_pico_init_adc();
void util_pedal_pico_start();
irq_handler_t util_pedal_pico_on_pwm_irq_wrap_handler_single();
void util_pedal_pico_stop();
void util_pedal_pico_remove_pwm_irq_exclusive_handler_on_core();
void util_pedal_pico_renew_adc_middle_moving_average(uint16 conversion);
void util_pedal_pico_on_adc_irq_fifo();
void util_pedal_pico_sw_loop(uchar8 gpio_1, uchar8 gpio_2); // Three Point Switch

#ifdef __cplusplus
}
#endif

#endif
