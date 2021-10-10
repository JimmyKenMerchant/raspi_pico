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

#define PEDAL_PICO_SUSTAIN_AMPLITUDE_SHIFT (10 + (UTIL_PEDAL_PICO_ADC_FINE_MULTIPLIER >> 1)) // Multiply by 2048 (0x00000000-0x0000F800)
#define PEDAL_PICO_SUSTAIN_PEAK_FIXED_1 127
#define PEDAL_PICO_SUSTAIN_PEAK_FIXED_2 255
#define PEDAL_PICO_SUSTAIN_PEAK_FIXED_3 511
#define PEDAL_PICO_SUSTAIN_GATE_HYSTERESIS_SHIFT 2 // Divide by 4
#define PEDAL_PICO_SUSTAIN_GATE_THRESHOLD_MULTIPLIER (1 * UTIL_PEDAL_PICO_ADC_FINE_MULTIPLIER) // From -60.20dB (Loss 1023) to -30.37dB (Loss 33) in 2047 as Quantization Peak
#define PEDAL_PICO_SUSTAIN_WAVE_MOVING_AVERAGE_NUMBER 8 // Should be Power of 2 Because of Processing Speed (Logical Shift Left on Division)

volatile util_pedal_pico* pedal_pico_sustain;
volatile uint16_t pedal_pico_sustain_conversion_2;
volatile uint16_t pedal_pico_sustain_conversion_3;
volatile int32_t pedal_pico_sustain_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
volatile uint16_t pedal_pico_sustain_gate_threshold;
volatile bool pedal_pico_sustain_is_on;
volatile int32_t pedal_pico_sustain_wave_moving_average_sum;

void pedal_pico_sustain_set();
void pedal_pico_sustain_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_pico_sustain_free();

#ifdef __cplusplus
}
#endif

#endif
