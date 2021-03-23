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
// Dependancies
#include "pico/stdlib.h"
// raspi_pico/include
#include "macros_pico.h"

/* Definitions */

#define SEQUENCER_GPIO_PICO_SEQUENCE_LENGTH_MAXIMUM 0xFFFF

#ifdef __cplusplus
extern "C" {
#endif

/* Structs */

typedef struct _sequencer_gpio_pico {
	uint32 pinlist_length;
	uchar8* pinlist;
	uint32 sequence_length;
	uint32* sequence;
	uint32 index;
} sequencer_gpio_pico;

/* Functions */

uint32 sequencer_gpio_pico_init();
uint32 sequencer_gpio_pico_execute(uint32 index);

#ifdef __cplusplus
}
#endif

#endif
