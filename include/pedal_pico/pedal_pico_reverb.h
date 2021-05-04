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

#define PEDAL_PICO_REVERB_SW_1_GPIO 14
#define PEDAL_PICO_REVERB_SW_2_GPIO 15
#define PEDAL_PICO_REVERB_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_PICO_REVERB_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_PICO_REVERB_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_PICO_REVERB_PWM_PEAK 2047
#define PEDAL_PICO_REVERB_GAIN 1
#define PEDAL_PICO_REVERB_DELAY_AMPLITUDE_PEAK (int32)(0x0000F000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PICO_REVERB_DELAY_AMPLITUDE_SHIFT 11
#define PEDAL_PICO_REVERB_DELAY_TIME_MAX 7937
#define PEDAL_PICO_REVERB_DELAY_TIME_SHIFT 8 // Multiply By 256 (0-7936), 7936 Divided by 28125 (0.28 Seconds)
#define PEDAL_PICO_REVERB_DELAY_TIME_INTERPOLATION_ACCUM 1

volatile util_pedal_pico* pedal_pico_reverb;
volatile uint16 pedal_pico_reverb_conversion_1;
volatile uint16 pedal_pico_reverb_conversion_2;
volatile uint16 pedal_pico_reverb_conversion_3;
volatile int16* pedal_pico_reverb_delay_array;
volatile int32 pedal_pico_reverb_delay_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
volatile uint16 pedal_pico_reverb_delay_time;
volatile uint16 pedal_pico_reverb_delay_time_interpolation;
volatile uint16 pedal_pico_reverb_delay_index;
volatile int32* pedal_pico_reverb_table_pdf_1;
volatile uint32 pedal_pico_reverb_debug_time;

void pedal_pico_reverb_start();
void pedal_pico_reverb_set();
void pedal_pico_reverb_on_pwm_irq_wrap();
void pedal_pico_reverb_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode);
void pedal_pico_reverb_free();

#ifdef __cplusplus
}
#endif

#endif
