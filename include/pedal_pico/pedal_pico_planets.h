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

#ifndef _PEDAL_PICO_PLANETS_H
#define _PEDAL_PICO_PLANETS_H 1

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

#define PEDAL_PICO_PLANETS_COEFFICIENT_FIXED_1 (int32_t)(0x00008000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PICO_PLANETS_DELAY_TIME_SHIFT (UTIL_PEDAL_PICO_ADC_COARSE_MULTIPLIER >> 1) // Multiply by 2 (1-32 to 2-64)
#define PEDAL_PICO_PLANETS_DELAY_TIME_MAX (((UTIL_PEDAL_PICO_ADC_COARSE_RESOLUTION + 1) << PEDAL_PICO_PLANETS_DELAY_TIME_SHIFT) + 1) // Don't Use Delay Time = 0
#define PEDAL_PICO_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_1 1 // Value to Accumulate, Have to Be Small Value with Moving Average

volatile util_pedal_pico* pedal_pico_planets;
volatile uint16_t pedal_pico_planets_conversion_2;
volatile uint16_t pedal_pico_planets_conversion_3;
volatile int16_t* pedal_pico_planets_delay_y;
volatile uint16_t pedal_pico_planets_delay_y_index;
volatile uint16_t pedal_pico_planets_delay_time_high_pass;
volatile uint16_t pedal_pico_planets_delay_time_high_pass_interpolation;
volatile uint16_t pedal_pico_planets_delay_time_low_pass;
volatile uint16_t pedal_pico_planets_delay_time_low_pass_interpolation;
volatile int32_t pedal_pico_planets_moving_average_sum_low_pass;

void pedal_pico_planets_set();
void pedal_pico_planets_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_pico_planets_free();

#ifdef __cplusplus
}
#endif

#endif
