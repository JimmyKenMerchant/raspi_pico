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

#ifndef _PEDAL_PICO_SUSTAIN_H
#define _PEDAL_PICO_SUSTAIN_H 1

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

#define PEDAL_PICO_SUSTAIN_DELAY_AMPLITUDE_SHIFT 11 // Multiply by 2048 (0x00000000-0x0000F800)
#define PEDAL_PICO_SUSTAIN_DELAY_TIME_FIXED_1 8
#define PEDAL_PICO_SUSTAIN_DELAY_TIME_MAX (PEDAL_PICO_SUSTAIN_DELAY_TIME_FIXED_1 + 1) // Don't Use Delay Time = 0
#define PEDAL_PICO_SUSTAIN_PEAK_FIXED_1 128
#define PEDAL_PICO_SUSTAIN_PEAK_FIXED_2 192
#define PEDAL_PICO_SUSTAIN_PEAK_FIXED_3 256
#define PEDAL_PICO_SUSTAIN_NOISE_GATE_HYSTERESIS_SHIFT 2 // Divide by 4
#define PEDAL_PICO_SUSTAIN_NOISE_GATE_THRESHOLD_MULTIPLIER 4

volatile util_pedal_pico* pedal_pico_sustain;
volatile uint16_t pedal_pico_sustain_conversion_2;
volatile uint16_t pedal_pico_sustain_conversion_3;
volatile int16_t* pedal_pico_sustain_delay_array;
volatile int32_t pedal_pico_sustain_delay_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part

volatile uint16_t pedal_pico_sustain_delay_time;
volatile uint16_t pedal_pico_sustain_delay_index;
volatile int8_t pedal_pico_sustain_noise_gate_threshold;
volatile bool pedal_pico_sustain_is_on;

void pedal_pico_sustain_set();
void pedal_pico_sustain_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_pico_sustain_free();

#ifdef __cplusplus
}
#endif

#endif
