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

#ifndef _FUNCTION_GENERATOR_PICO_H
#define _FUNCTION_GENERATOR_PICO_H 1

// Standards
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// Dependancies
#include "pico/stdlib.h"
#include "pico/divider.h"
#include "pico/float.h"
// raspi_pico/include
#include "macros_pico.h"
#include "constants_science_pico.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions */

#define FUNCTION_GENERATOR_PICO_SEQUENCE_LENGTH_MAXIMUM 0xFFFF

/* Structs */

typedef struct {
    uint16_t width;
    uint16_t amplitude;
    uint32_t time; // Time in Width * 2^factor
    uint8_t factor; // Width * 2^factor
    bool is_end;
} function_generator_pico;

/* Functions */
function_generator_pico* function_generator_pico_init(uint16_t width, uint8_t factor, uint16_t amplitude);
int16_t function_generator_pico_sine(function_generator_pico* function_generator);

#ifdef __cplusplus
}
#endif

#endif
