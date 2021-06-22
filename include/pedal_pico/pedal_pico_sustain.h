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
#define PEDAL_PICO_SUSTAIN_NOISE_GATE_HYSTERESIS_SHIFT 2 // Divide by 4
#define PEDAL_PICO_SUSTAIN_NOISE_GATE_THRESHOLD_MULTIPLIER 4
#define PEDAL_PICO_SUSTAIN_NOISE_GATE_COUNT_MAX 4000 // 28125 Divided by 2000 = Approx. 14Hz

volatile util_pedal_pico* pedal_pico_sustain;
volatile uint16 pedal_pico_sustain_conversion_2;
volatile uint16 pedal_pico_sustain_conversion_3;
volatile int16* pedal_pico_sustain_delay_array;
volatile int32 pedal_pico_sustain_delay_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part

volatile uint16 pedal_pico_sustain_delay_time;
volatile uint16 pedal_pico_sustain_delay_index;
volatile char8 pedal_pico_sustain_noise_gate_threshold;
volatile uint16 pedal_pico_sustain_noise_gate_count;
volatile bool pedal_pico_sustain_is_on;
volatile int16 pedal_pico_sustain_wave;

void pedal_pico_sustain_set();
void pedal_pico_sustain_process(int32 normalized_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode);
void pedal_pico_sustain_free();

#ifdef __cplusplus
}
#endif

#endif
