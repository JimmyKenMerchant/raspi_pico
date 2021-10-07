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

#ifndef _PEDAL_PICO_TREMOLO_H
#define _PEDAL_PICO_TREMOLO_H 1

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

#define PEDAL_PICO_TREMOLO_OSC_TRIANGLE_1_TIME_MULTIPLIER (216 / UTIL_PEDAL_PICO_ADC_COARSE_MULTIPLIER)
#define PEDAL_PICO_TREMOLO_OSC_START_HYSTERESIS_SHIFT 1 // Divide by 2
#define PEDAL_PICO_TREMOLO_OSC_START_THRESHOLD_MULTIPLIER (1 * UTIL_PEDAL_PICO_ADC_COARSE_MULTIPLIER) // Up to -30.37dB (Loss 33) in 2047 as Quantization Peak
#define PEDAL_PICO_TREMOLO_OSC_START_COUNT_MAX 2000 // 28125 Divided by 2000 = Approx. 14Hz

volatile util_pedal_pico* pedal_pico_tremolo;
volatile uint16_t pedal_pico_tremolo_conversion_2;
volatile uint16_t pedal_pico_tremolo_conversion_3;
volatile int32_t pedal_pico_tremolo_osc_triangle_1_index; // May Have Negative Value Depending on osc_speed
volatile uint16_t pedal_pico_tremolo_osc_speed;
volatile bool pedal_pico_tremolo_osc_is_negative;
volatile uint16_t pedal_pico_tremolo_osc_start_threshold;
volatile uint16_t pedal_pico_tremolo_osc_start_count;
volatile bool pedal_pico_tremolo_osc_is_faded;

void pedal_pico_tremolo_set();
void pedal_pico_tremolo_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_pico_tremolo_free();

#ifdef __cplusplus
}
#endif

#endif
