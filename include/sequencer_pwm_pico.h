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

#ifndef _SEQUENCER_PWM_PICO_H
#define _SEQUENCER_PWM_PICO_H 1

// Standards
#include <stdio.h>
#include <stdlib.h>
// Dependancies
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
// raspi_pico/include
#include "macros_pico.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions */
#define SEQUENCER_PWM_PICO_SEQUENCE_LENGTH_MAXIMUM 0xFFFF

/* Structs */
typedef struct {
    uint8_t slice; // Bit[7] Clear for A, Set for B. Bit[6:0] Slice Number
    // Clearing MSB in sequencer_pwm_pico->sequence[] shows the end of the sequence.
    // That is the resolution is 32768.
    uint16_t* sequence;
    uint32_t sequence_length; // Length of sequences
    uint32_t index;
    uint16_t sequence_interpolation; // Accumulated Value in Interpolation
    uint16_t sequence_interpolation_accum; // Unit Value to Accumlate in Interpolation. 0 is no interpolation.
} sequencer_pwm_pico;

/* Functions */
sequencer_pwm_pico* sequencer_pwm_pico_init(uint8_t slice, uint16_t* sequence);
uint32_t sequencer_pwm_pico_get_sequence_length(uint16_t* sequence);
bool sequencer_pwm_pico_execute(sequencer_pwm_pico* sequencer_pwm);

#ifdef __cplusplus
}
#endif

#endif
