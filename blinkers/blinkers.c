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

#include <stdio.h>
#include "pico/stdlib.h"
#include "sequencer_gpio.h"

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // Wait for Rediness of USB for Messages
    sequencer_gpio_init();
    unsigned long index = 0;
    printf("Let's Start!\n");
    while (true) {
        printf("In The Loop: %d\n", index);
        index = sequencer_gpio_execute(index);
        sleep_ms(500);
    }
    return 0;
}
