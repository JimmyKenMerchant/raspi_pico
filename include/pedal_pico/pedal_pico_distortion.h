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

#ifndef _PEDAL_PICO_DISTORTION_H
#define _PEDAL_PICO_DISTORTION_H 1

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

#define PEDAL_PICO_DISTORTION_GAIN 1
#define PEDAL_PICO_DISTORTION_CUTOFF_FIXED_1 0xC0

volatile util_pedal_pico* pedal_pico_distortion;
volatile uint16 pedal_pico_distortion_conversion_1;
volatile uint16 pedal_pico_distortion_conversion_2;
volatile uint16 pedal_pico_distortion_conversion_3;
volatile uint16 pedal_pico_distortion_loss;
volatile int32* pedal_pico_distortion_table_pdf_1;
volatile int32* pedal_pico_distortion_table_log_1;
volatile int32* pedal_pico_distortion_table_log_2;
volatile int32* pedal_pico_distortion_table_power_1;

void pedal_pico_distortion_set();
void pedal_pico_distortion_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3, uchar8 sw_mode);
void pedal_pico_distortion_free();

#ifdef __cplusplus
}
#endif

#endif