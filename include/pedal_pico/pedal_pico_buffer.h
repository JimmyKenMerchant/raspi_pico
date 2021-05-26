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

#define PEDAL_PICO_BUFFER_DELAY_TIME_MAX 7937
#define PEDAL_PICO_BUFFER_DELAY_TIME_SHIFT 8 // Multiply By 256 (0-7936), 7936 Divided by 28125 (0.282 Seconds)
#define PEDAL_PICO_BUFFER_DELAY_TIME_INTERPOLATION_ACCUM 1
#define PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_1 (int32)(0x00004000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_2 (int32)(0x00008000)
#define PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_FIXED_3 (int32)(0x0000F000)
#define PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_1 0x2
#define PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_2 0x4
#define PEDAL_PICO_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_3 0x8
#define PEDAL_PICO_BUFFER_NOISE_GATE_REDUCE_SHIFT 6 // Divide by 64
#define PEDAL_PICO_BUFFER_NOISE_GATE_THRESHOLD_MULTIPLIER 1 // From -66.22dB (Loss 2047) to -36.39dB (Loss 66) in ADC_VREF (Typically 3.3V)
#define PEDAL_PICO_BUFFER_NOISE_GATE_COUNT_MAX 2000 // 28125 Divided by 2000 = Approx. 14Hz

volatile util_pedal_pico* pedal_pico_buffer;
volatile uint16 pedal_pico_buffer_conversion_1;
volatile uint16 pedal_pico_buffer_conversion_2;
volatile uint16 pedal_pico_buffer_conversion_3;
volatile int16* pedal_pico_buffer_delay_array;
volatile int32 pedal_pico_buffer_delay_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
volatile int32 pedal_pico_buffer_delay_amplitude_interpolation;
volatile uchar8 pedal_pico_buffer_delay_amplitude_interpolation_accum;
volatile uint16 pedal_pico_buffer_delay_time;
volatile uint16 pedal_pico_buffer_delay_time_interpolation;
volatile uint16 pedal_pico_buffer_delay_index;
volatile char8 pedal_pico_buffer_noise_gate_threshold;
volatile uint16 pedal_pico_buffer_noise_gate_count;

void pedal_pico_buffer_set();
void pedal_pico_buffer_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode);
void pedal_pico_buffer_free();

#ifdef __cplusplus
}
#endif

#endif
