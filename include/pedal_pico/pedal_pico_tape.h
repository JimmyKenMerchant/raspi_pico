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

#ifndef _PEDAL_PICO_TAPE_H
#define _PEDAL_PICO_TAPE_H 1

// Standards
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// Dependancies
#include "pico/stdlib.h"
#include "pico/divider.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
// raspi_pico/include
#include "macros_pico.h"
#include "util_pedal_pico.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PEDAL_PICO_TAPE_TRANSIENT_RESPONSE 100000 // 100000 Micro Seconds
#define PEDAL_PICO_TAPE_CORE_1_STACK_SIZE 1024 * 4 // 1024 Words, 4096 Bytes
#define PEDAL_PICO_TAPE_LED_GPIO 25
#define PEDAL_PICO_TAPE_SW_1_GPIO 14
#define PEDAL_PICO_TAPE_SW_2_GPIO 15
#define PEDAL_PICO_TAPE_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_PICO_TAPE_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_PICO_TAPE_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_PICO_TAPE_PWM_PEAK 2047
#define PEDAL_PICO_TAPE_GAIN 1
#define PEDAL_PICO_TAPE_DELAY_AMPLITUDE_PEAK_FIXED_1 (int32)(0x00008000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PICO_TAPE_DELAY_TIME_MAX 3969
#define PEDAL_PICO_TAPE_DELAY_TIME_FIXED_1 1984 // 1920 Divided by 28125 (0.068 Seconds)
#define PEDAL_PICO_TAPE_DELAY_TIME_SWING_PEAK_1 1984
#define PEDAL_PICO_TAPE_DELAY_TIME_SWING_SHIFT 6 // Multiply By 64 (0-1984)
#define PEDAL_PICO_TAPE_OSC_SINE_1_TIME_MAX 9375
#define PEDAL_PICO_TAPE_OSC_SINE_1_TIME_MULTIPLIER 6

volatile util_pedal_pico* pedal_pico_tape;
volatile uint16 pedal_pico_tape_conversion_1;
volatile uint16 pedal_pico_tape_conversion_2;
volatile uint16 pedal_pico_tape_conversion_3;
volatile uint32 pedal_pico_tape_osc_sine_1_index;
volatile uint16 pedal_pico_tape_osc_speed;
volatile bool pedal_pico_tape_osc_is_negative;
volatile int16* pedal_pico_tape_delay_array;
volatile int32 pedal_pico_tape_delay_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
volatile uint16 pedal_pico_tape_delay_time;
volatile uint16 pedal_pico_tape_delay_index;
volatile uint16 pedal_pico_tape_delay_time_swing;
volatile int32* pedal_pico_tape_table_pdf_1;
volatile int32* pedal_pico_tape_table_sine_1;
volatile uint32 pedal_pico_tape_debug_time;

void pedal_pico_tape_start();
void pedal_pico_tape_set();
void pedal_pico_tape_on_pwm_irq_wrap();
void pedal_pico_tape_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode);
void pedal_pico_tape_free();

#ifdef __cplusplus
}
#endif

#endif
