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

#ifndef _PEDAL_PICO_REVERB_H
#define _PEDAL_PICO_REVERB_H 1

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

#define PEDAL_PICO_REVERB_DELAY_AMPLITUDE_SHIFT (10 + (UTIL_PEDAL_PICO_ADC_FINE_MULTIPLIER >> 1)) // Multiply by 1024 (0x00000000-0x0000FC00)
#define PEDAL_PICO_REVERB_DELAY_RESONANCE (int32_t)((0x00010000 - (UTIL_PEDAL_PICO_ADC_FINE_RESOLUTION << PEDAL_PICO_REVERB_DELAY_AMPLITUDE_SHIFT)) + 0x00000010)
#define PEDAL_PICO_REVERB_DELAY_TIME_SHIFT (8 + (UTIL_PEDAL_PICO_ADC_FINE_MULTIPLIER >> 1)) // Multiply by 256 (0-16128), 16128 Divided by 28125 (0.57 Seconds)
#define PEDAL_PICO_REVERB_DELAY_TIME_MAX ((UTIL_PEDAL_PICO_ADC_FINE_RESOLUTION << PEDAL_PICO_REVERB_DELAY_TIME_SHIFT) + 1) // Don't Use Delay Time = 0
#define PEDAL_PICO_REVERB_DELAY_TIME_INTERPOLATION_ACCUM 1
#define PEDAL_PICO_REVERB_WAVE_MOVING_AVERAGE_NUMBER 8 // Should be Power of 2 Because of Processing Speed (Logical Shift Left on Division)

volatile util_pedal_pico* pedal_pico_reverb;
volatile uint16_t pedal_pico_reverb_conversion_2;
volatile uint16_t pedal_pico_reverb_conversion_3;
volatile int16_t* pedal_pico_reverb_delay_array;
volatile int32_t pedal_pico_reverb_delay_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
volatile uint16_t pedal_pico_reverb_delay_time;
volatile uint16_t pedal_pico_reverb_delay_time_interpolation;
volatile uint16_t pedal_pico_reverb_delay_index;
volatile int32_t pedal_pico_reverb_wave_moving_average_sum;

void pedal_pico_reverb_set();
void pedal_pico_reverb_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_pico_reverb_free();

#ifdef __cplusplus
}
#endif

#endif
