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

#ifndef _SEQUENCER_GPIO_PICO_H
#define _SEQUENCER_GPIO_PICO_H 1

// Standards
#include <stdio.h>
#include <stdlib.h>
// Dependancies
#include "pico/stdlib.h"
// raspi_pico/include
#include "macros_pico.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions */

#define SEQUENCER_GPIO_PICO_SEQUENCE_LENGTH_MAXIMUM 0xFFFF

/* Structs */

typedef struct {
	uint32 pinlist_length;
	uchar8* pinlist;
	uint32 sequence_length;
	// LSB in sequencer_gpio_pico->sequence[] is the last in sequencer_gpio_pico->pinlist.
	// Clearing MSB in sequencer_gpio_pico->sequence[] shows the end of the sequence.
	uint16* sequence;
	uint32 index;
} sequencer_gpio_pico;

/* Functions */

sequencer_gpio_pico* sequencer_gpio_pico_init(uchar8* pinlist, uint32 pinlist_length, uint16* sequence);
uint32 sequencer_gpio_pico_get_sequence_length(uint16* sequence);
bool sequencer_gpio_pico_execute(sequencer_gpio_pico* sequencer_gpio);

#ifdef __cplusplus
}
#endif

#endif
