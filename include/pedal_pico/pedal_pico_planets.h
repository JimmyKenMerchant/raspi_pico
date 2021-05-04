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

#define PEDAL_PICO_PLANETS_SW_1_GPIO 14
#define PEDAL_PICO_PLANETS_SW_2_GPIO 15
#define PEDAL_PICO_PLANETS_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_PICO_PLANETS_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_PICO_PLANETS_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_PICO_PLANETS_PWM_PEAK 2047
#define PEDAL_PICO_PLANETS_GAIN 1
#define PEDAL_PICO_PLANETS_COEFFICIENT_PEAK (int32)(0x00010000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PICO_PLANETS_COEFFICIENT_SHIFT 10 // Multiply by 2048 (0x00000800-0x00007C00)
#define PEDAL_PICO_PLANETS_COEFFICIENT_INTERPOLATION_ACCUM 0x80 // Value to Accumulate
#define PEDAL_PICO_PLANETS_DELAY_TIME_MAX 2049 // Don't Use Delay Time = 0
#define PEDAL_PICO_PLANETS_DELAY_TIME_SHIFT 6 // Multiply by 64 (64-2048)
#define PEDAL_PICO_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_1 1 // Value to Accumulate, Small Value Makes Froggy
#define PEDAL_PICO_PLANETS_DELAY_TIME_INTERPOLATION_ACCUM_FIXED_2 16 // Value to Accumulate

volatile util_pedal_pico* pedal_pico_planets;
volatile uint16 pedal_pico_planets_conversion_1;
volatile uint16 pedal_pico_planets_conversion_2;
volatile uint16 pedal_pico_planets_conversion_3;
volatile int32 pedal_pico_planets_coefficient;
volatile int32 pedal_pico_planets_coefficient_interpolation;
volatile int16* pedal_pico_planets_delay_x;
volatile int16* pedal_pico_planets_delay_y;
volatile uint16 pedal_pico_planets_delay_time;
volatile uint16 pedal_pico_planets_delay_time_interpolation;
volatile uint16 pedal_pico_planets_delay_time_interpolation_accum;
volatile uint16 pedal_pico_planets_delay_index;
volatile int32* pedal_pico_planets_table_pdf_1;
volatile uint32 pedal_pico_planets_debug_time;

void pedal_pico_planets_start();
void pedal_pico_planets_set();
void pedal_pico_planets_on_pwm_irq_wrap();
void pedal_pico_planets_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode);
void pedal_pico_planets_free();

#ifdef __cplusplus
}
#endif

#endif
