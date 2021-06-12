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

#ifndef _PEDAL_PICO_CHORUS_H
#define _PEDAL_PICO_CHORUS_H 1

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

#define PEDAL_PICO_CHORUS_DELAY_AMPLITUDE_FIXED_1 (int32)(0x00010000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PICO_CHORUS_DELAY_TIME_FIXED_1 1526 // 1526 Divided by 28125 (0.054 Seconds)
#define PEDAL_PICO_CHORUS_DELAY_TIME_FIXED_2 (PEDAL_PICO_CHORUS_DELAY_TIME_FIXED_1 * 2) // 0.108 Seconds
#define PEDAL_PICO_CHORUS_DELAY_TIME_FIXED_3 (PEDAL_PICO_CHORUS_DELAY_TIME_FIXED_1 * 4) // 0.216 Seconds
#define PEDAL_PICO_CHORUS_DELAY_TIME_MAX (PEDAL_PICO_CHORUS_DELAY_TIME_FIXED_3 + 1) // Don't Use Delay Time = 0
#define PEDAL_PICO_CHORUS_OSC_SINE_1_TIME_MULTIPLIER 6
#define PEDAL_PICO_CHORUS_LR_DISTANCE_TIME_SHIFT 5 // Multiply By 32 (0-992), 992 Divided by 28125 (0.0353 Seconds = 12.01 Meters)
#define PEDAL_PICO_CHORUS_LR_DISTANCE_TIME_MAX ((UTIL_PEDAL_PICO_ADC_RESOLUTION << PEDAL_PICO_CHORUS_LR_DISTANCE_TIME_SHIFT) + 1)
#define PEDAL_PICO_CHORUS_LR_DISTANCE_TIME_INTERPOLATION_ACCUM 1 // Value to Accumulate

volatile util_pedal_pico* pedal_pico_chorus;
volatile uint16 pedal_pico_chorus_conversion_2;
volatile uint16 pedal_pico_chorus_conversion_3;
volatile uint32 pedal_pico_chorus_osc_sine_1_index;
volatile uint16 pedal_pico_chorus_osc_speed;
volatile int16* pedal_pico_chorus_delay_array;
volatile int32 pedal_pico_chorus_delay_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
volatile uint16 pedal_pico_chorus_delay_time;
volatile uint16 pedal_pico_chorus_delay_index;
volatile int16* pedal_pico_chorus_lr_distance_array;
volatile uint16 pedal_pico_chorus_lr_distance_time;
volatile uint16 pedal_pico_chorus_lr_distance_time_interpolation;
volatile uint16 pedal_pico_chorus_lr_distance_index;

void pedal_pico_chorus_set();
void pedal_pico_chorus_process(int32 normalized_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode);
void pedal_pico_chorus_free();

#ifdef __cplusplus
}
#endif

#endif
