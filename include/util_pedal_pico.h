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

#ifndef _UTIL_PEDAL_PICO_H
#define _UTIL_PEDAL_PICO_H 1

// Standards
#include <stdio.h>
// Dependancies
#include "pico/stdlib.h"
#include "pico/divider.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/resets.h"
// raspi_pico/include
#include "macros_pico.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions */

#define UTIL_PEDAL_PICO_ADC_0_GPIO 26
#define UTIL_PEDAL_PICO_ADC_1_GPIO 27
#define UTIL_PEDAL_PICO_ADC_2_GPIO 28

#define UTIL_PEDAL_PICO_SW_THRESHOLD 30
#define UTIL_PEDAL_PICO_SW_SLEEP_TIME 1000
#define util_pedal_pico_cutoff_normalized(x, y) (_max(-y, _min(x, y))) // X: Value, Y: Absolute Peak
#define util_pedal_pico_cutoff_biased(x, y, z) (_max(z, _min(x, y))) // X: Value, Y: Peak, Z: Bottom

/* Global Variables */

volatile uint16 util_pedal_pico_on_adc_conversion_1;
volatile uint16 util_pedal_pico_on_adc_conversion_2;
volatile uint16 util_pedal_pico_on_adc_conversion_3;
volatile bool util_pedal_pico_on_adc_is_outstanding;
volatile uchar8 util_pedal_pico_sw_mode;

/* Functions */

void util_pedal_pico_init_adc();
void util_pedal_pico_on_adc_irq_fifo();
void util_pedal_pico_sw_loop(uchar8 gpio_1, uchar8 gpio_2); // Three Point Switch

#ifdef __cplusplus
}
#endif

#endif