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

#ifndef _PEDAL_PICO_ENVELOPE_H
#define _PEDAL_PICO_ENVELOPE_H 1

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

#define PEDAL_PICO_ENVELOPE_DECAY_LIMIT_FIXED_1 (int32_t)(0x00004000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PICO_ENVELOPE_DECAY_LIMIT_FIXED_2 (int32_t)(0x00008000)
#define PEDAL_PICO_ENVELOPE_DECAY_LIMIT_FIXED_3 (int32_t)(0x0000C000)
#define PEDAL_PICO_ENVELOPE_DECAY_TRIANGLE_1_TIME_MULTIPLIER (12 / UTIL_PEDAL_PICO_ADC_FINE_MULTIPLIER)
#define PEDAL_PICO_ENVELOPE_DECAY_HYSTERESIS_SHIFT 0
#define PEDAL_PICO_ENVELOPE_DECAY_THRESHOLD_MULTIPLIER (16 * UTIL_PEDAL_PICO_ADC_FINE_MULTIPLIER) // Up to 1008 in 2047 as Quantization Peak
/**
 * Maximum Frequency on Enveloping:
 * (Sample Rate Divided by
 * (((PEDAL_PICO_ENVELOPE_DECAY_TRIANGLE_1_TIME_MULTIPLIER * UTIL_PEDAL_PICO_OSC_TRIANGLE_1_TIME_MAX))
 * Divided by UTIL_PEDAL_PICO_ADC_FINE_RESOLUTION)) Divided by 2 for Decay-Release
 *
 * Thus:
 * (28125 Divided by ((12 * 3125) Divided by 64)) Divided by 2 = 24Hz
 *
 * PEDAL_PICO_ENVELOPE_DECAY_COUNT_MAX needs value to exceed the maximum frequency on enveloping.
 */
#define PEDAL_PICO_ENVELOPE_DECAY_COUNT_MAX 75 // 28125 Divided by 75 = 375Hz

volatile util_pedal_pico* pedal_pico_envelope;
volatile uint16_t pedal_pico_envelope_conversion_2;
volatile uint16_t pedal_pico_envelope_conversion_3;
volatile int32_t pedal_pico_envelope_decay_triangle_1_index; // May Have Negative Value Depending on osc_speed
volatile uint16_t pedal_pico_envelope_decay_speed;
volatile bool pedal_pico_envelope_decay_is_release;
volatile uint16_t pedal_pico_envelope_decay_start_threshold;
volatile uint16_t pedal_pico_envelope_decay_start_count;
volatile bool pedal_pico_envelope_decay_is_outgoing;

void pedal_pico_envelope_set();
void pedal_pico_envelope_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_pico_envelope_free();

#ifdef __cplusplus
}
#endif

#endif
