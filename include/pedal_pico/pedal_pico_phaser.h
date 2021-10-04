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

#ifndef _PEDAL_PICO_PHASER_H
#define _PEDAL_PICO_PHASER_H 1

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

#define PEDAL_PICO_PHASER_COEFFICIENT_PEAK_FIXED_1 (int32_t)(0x00010000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PICO_PHASER_DELAY_TIME_FIXED_1 64 // 28125 Divided by 64 (439.45Hz)
#define PEDAL_PICO_PHASER_DELAY_TIME_FIXED_2 (PEDAL_PICO_PHASER_DELAY_TIME_FIXED_1 * 2) // 28125 Divided by 128 (219.72Hz)
#define PEDAL_PICO_PHASER_DELAY_TIME_FIXED_3 (PEDAL_PICO_PHASER_DELAY_TIME_FIXED_1 * 4) // 28125 Divided by 256 (109.86Hz)
#define PEDAL_PICO_PHASER_DELAY_TIME_MAX (PEDAL_PICO_PHASER_DELAY_TIME_FIXED_3 + 1) // Don't Use Delay Time = 0
#define PEDAL_PICO_PHASER_OSC_TRIANGLE_1_TIME_MULTIPLIER 108
#define PEDAL_PICO_PHASER_OSC_START_HYSTERESIS_SHIFT 1 // Divide by 2
#define PEDAL_PICO_PHASER_OSC_START_THRESHOLD_MULTIPLIER 1 // From -66.22dB (Loss 2047) to -36.39dB (Loss 66) in ADC_VREF (Typically 3.3V)
#define PEDAL_PICO_PHASER_OSC_START_COUNT_MAX 2000 // 28125 Divided by 2000 = Approx. 14Hz

volatile util_pedal_pico* pedal_pico_phaser;
volatile uint16_t pedal_pico_phaser_conversion_2;
volatile uint16_t pedal_pico_phaser_conversion_3;
volatile int16_t* pedal_pico_phaser_delay_x;
volatile int16_t* pedal_pico_phaser_delay_y;
volatile uint16_t pedal_pico_phaser_delay_time;
volatile uint16_t pedal_pico_phaser_delay_index;
volatile int32_t pedal_pico_phaser_osc_triangle_1_index; // May Have Negative Value Depending on osc_speed
volatile uint16_t pedal_pico_phaser_osc_speed;
volatile bool pedal_pico_phaser_osc_is_negative;
volatile uint16_t pedal_pico_phaser_osc_start_threshold;
volatile uint16_t pedal_pico_phaser_osc_start_count;

void pedal_pico_phaser_set();
void pedal_pico_phaser_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_pico_phaser_free();

#ifdef __cplusplus
}
#endif

#endif
