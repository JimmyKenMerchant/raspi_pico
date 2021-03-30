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

#include "function_generator_pico.h"

function_generator_pico* function_generator_pico_init(uint16 width, uint16 factor, uint16 amplitude) {
    function_generator_pico* function_generator = (function_generator_pico*)malloc(sizeof(function_generator_pico));
    if (function_generator == null) return function_generator;
    function_generator->width = width;
    function_generator->factor = factor;
    function_generator->amplitude = amplitude;
    function_generator->time = 0;
    function_generator->is_end = false;
    return function_generator;
}

int16 function_generator_pico_sine(function_generator_pico* function_generator) {
    if (function_generator == null) return 0;
    uint32 width_factored = function_generator->width << function_generator->factor;
    if (function_generator->time >= width_factored) function_generator->time = 0;
    float32 radius = ((float32)function_generator->time / (float32)width_factored) * 2.0f * CONSTANTS_SCIENCE_PICO_PI_FLOAT;
    function_generator->time++;
    if (function_generator->time == width_factored) function_generator->is_end = true;
    return (int16)(sinf(radius) * (float32)function_generator->amplitude);
}
