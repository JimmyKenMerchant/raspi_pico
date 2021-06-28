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

#ifndef _PEDAL_PICO_SIDEBAND_H
#define _PEDAL_PICO_SIDEBAND_H 1

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

#define PEDAL_PICO_SIDEBAND_CUTOFF_FIXED_1 0xFF // 8-bit (Normalized 0 to peak)
#define PEDAL_PICO_SIDEBAND_GAIN_SHIFT_FIXED_1 8
#define PEDAL_PICO_SIDEBAND_OSC_SINE_1_TIME_MULTIPLIER 6
#define PEDAL_PICO_SIDEBAND_OSC_SINE_2_TIME_MULTIPLIER 4
#define PEDAL_PICO_SIDEBAND_OSC_PEAK 1023
#define PEDAL_PICO_SIDEBAND_OSC_START_HYSTERESIS_SHIFT 1 // Divide by 2
#define PEDAL_PICO_SIDEBAND_OSC_START_THRESHOLD_MULTIPLIER 1 // From -66.22dB (Loss 2047) to -36.39dB (Loss 66) in ADC_VREF (Typically 3.3V)
#define PEDAL_PICO_SIDEBAND_OSC_START_COUNT_MAX 2000 // 28125 Divided by 2000 = Approx. 14Hz
#define PEDAL_PICO_SIDEBAND_MIDDLE_MOVING_AVERAGE_NUMBER 2048 // Should be Power of 2 Because of Processing Speed (Logical Shift Left on Division)
#define PEDAL_PICO_SIDEBAND_WAVE_MOVING_AVERAGE_NUMBER 8 // Should be Power of 2 Because of Processing Speed (Logical Shift Left on Division)

volatile util_pedal_pico* pedal_pico_sideband;
volatile uint16_t pedal_pico_sideband_conversion_2;
volatile uint16_t pedal_pico_sideband_conversion_3;
volatile uint32_t pedal_pico_sideband_osc_sine_1_index;
volatile uint32_t pedal_pico_sideband_osc_sine_2_index;
volatile uint16_t pedal_pico_sideband_osc_speed;
volatile int8_t pedal_pico_sideband_osc_start_threshold;
volatile uint16_t pedal_pico_sideband_osc_start_count;
volatile int32_t pedal_pico_sideband_middle_moving_average_sum;
volatile int32_t pedal_pico_sideband_wave_moving_average_sum;

void pedal_pico_sideband_set();
void pedal_pico_sideband_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_pico_sideband_free();

#ifdef __cplusplus
}
#endif

#endif
