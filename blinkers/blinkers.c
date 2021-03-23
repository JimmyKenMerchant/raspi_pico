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

// Standards
#include <stdio.h>
// Dependancies
#include "pico/stdlib.h"
// raspi_pico/include
#include "macros_pico.h"
#include "sequencer_gpio_pico.h"

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // Wait for Rediness of USB for Messages
    sequencer_gpio_pico_init();
    uint32 index = 0;
    printf("Let's Start!\n");
    while (true) {
        printf("In The Loop: %d\n", index);
        index = sequencer_gpio_pico_execute(index);
        sleep_ms(500);
    }
    return 0;
}
