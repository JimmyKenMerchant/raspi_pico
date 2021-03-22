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

#ifndef _SEQUENCER_GPIO_
#define _SEQUENCER_GPIO_
#endif

#include <stdio.h>
#include "pico/stdlib.h"

unsigned long sequencer_gpio_init();
unsigned long sequencer_gpio_execute(unsigned long index);

#define SEQUENCER_GPIO_COMMAND_LENGTH_MAXIMUM 0xFFFF
