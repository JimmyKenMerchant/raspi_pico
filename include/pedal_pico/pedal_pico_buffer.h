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

#ifndef _PEDAL_PICO_BUFFER_H
#define _PEDAL_PICO_BUFFER_H 1

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

#define PEDAL_PICO_BUFFER_DELAY_TIME_SHIFT (7 + (UTIL_PEDAL_PICO_ADC_FINE_MULTIPLIER >> 1)) // Multiply By 256 (0-7936), 7936 Divided by 28125 (0.282 Seconds)
#define PEDAL_PICO_BUFFER_DELAY_TIME_MAX ((UTIL_PEDAL_PICO_ADC_FINE_RESOLUTION << PEDAL_PICO_BUFFER_DELAY_TIME_SHIFT) + 1) // Don't Use Delay Time = 0
#define PEDAL_PICO_BUFFER_DELAY_TIME_INTERPOLATION_ACCUM 1
#define PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_1 (int32_t)(0x00004000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_2 (int32_t)(0x00008000)
#define PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_3 (int32_t)(0x0000F000)
#define PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_1 0x2
#define PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_2 0x4
#define PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_3 0x8
#define PEDAL_PICO_BUFFER_NOISE_GATE_HYSTERESIS_SHIFT 1 // Divide by 2
#define PEDAL_PICO_BUFFER_NOISE_GATE_THRESHOLD_MULTIPLIER (1 * UTIL_PEDAL_PICO_ADC_FINE_MULTIPLIER) // Up to -30.37dB (Loss 33) in 2047 as Quantization Peak
#define PEDAL_PICO_BUFFER_NOISE_GATE_COUNT_MAX 2000 // 28125 Divided by 2000 = Approx. 14Hz

volatile util_pedal_pico* pedal_pico_buffer;
volatile uint16_t pedal_pico_buffer_conversion_2;
volatile uint16_t pedal_pico_buffer_conversion_3;
volatile int16_t* pedal_pico_buffer_delay_array;
volatile int32_t pedal_pico_buffer_delay_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
volatile int32_t pedal_pico_buffer_delay_amplitude_interpolation;
volatile uint8_t pedal_pico_buffer_delay_amplitude_interpolation_accum;
volatile uint16_t pedal_pico_buffer_delay_time;
volatile uint16_t pedal_pico_buffer_delay_time_interpolation;
volatile uint16_t pedal_pico_buffer_delay_index;
volatile uint16_t pedal_pico_buffer_noise_gate_threshold;
volatile uint16_t pedal_pico_buffer_noise_gate_count;

void pedal_pico_buffer_set();
void pedal_pico_buffer_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_pico_buffer_free();

#ifdef __cplusplus
}
#endif

#endif
