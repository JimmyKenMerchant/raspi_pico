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

#define PEDAL_PICO_TAPE_DELAY_AMPLITUDE_PEAK_FIXED_1 (int32_t)(0x00008000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PICO_TAPE_DELAY_TIME_SWING_SHIFT (5 + (UTIL_PEDAL_PICO_ADC_COARSE_MULTIPLIER >> 1)) // Multiply by 64 (0-1984)
#define PEDAL_PICO_TAPE_DELAY_TIME_SWING_PEAK_1 (UTIL_PEDAL_PICO_ADC_COARSE_RESOLUTION << PEDAL_PICO_TAPE_DELAY_TIME_SWING_SHIFT) // 1984 Divided by 28125 (0.070 Seconds)
#define PEDAL_PICO_TAPE_DELAY_TIME_FIXED_1 PEDAL_PICO_TAPE_DELAY_TIME_SWING_PEAK_1
#define PEDAL_PICO_TAPE_DELAY_TIME_MAX (PEDAL_PICO_TAPE_DELAY_TIME_SWING_PEAK_1 + PEDAL_PICO_TAPE_DELAY_TIME_FIXED_1 + 1) // Don't Use Delay Time = 0
#define PEDAL_PICO_TAPE_OSC_SINE_1_TIME_MULTIPLIER (18 / UTIL_PEDAL_PICO_ADC_COARSE_MULTIPLIER)

volatile util_pedal_pico* pedal_pico_tape;
volatile uint16_t pedal_pico_tape_conversion_2;
volatile uint16_t pedal_pico_tape_conversion_3;
volatile uint32_t pedal_pico_tape_osc_sine_1_index;
volatile uint16_t pedal_pico_tape_osc_speed;
volatile bool pedal_pico_tape_osc_is_negative;
volatile int16_t* pedal_pico_tape_delay_array;
volatile int32_t pedal_pico_tape_delay_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
volatile uint16_t pedal_pico_tape_delay_time;
volatile uint16_t pedal_pico_tape_delay_index;
volatile uint16_t pedal_pico_tape_delay_time_swing;

void pedal_pico_tape_set();
void pedal_pico_tape_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_pico_tape_free();

#ifdef __cplusplus
}
#endif

#endif
