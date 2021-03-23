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

uchar8 pinlist[] = {15,16,25}; // Pin No. 0 to No. 29
uint32 pinlist_length = 3;
// LSB in sequencer_gpio_pico->sequence is the last in sequencer_gpio_pico->pinlist.
uint16 sequence[] = {0b1000000000000111,
                     0b1000000000000101,
                     0b1000000000000011,
                     0b1000000000000101,
                     0b1000000000000011,
                     0b1000000000000110,
                     0b0000000000000000, // Clear MSB to Show End
                     };

sequencer_gpio_pico* the_sequencer;

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // Wait for Rediness of USB for Messages
    the_sequencer = sequencer_gpio_pico_init(pinlist, pinlist_length, sequence);
    uint32 index = 0;
    printf("Let's Start!\n");
    while (true) {
        printf("In The Loop: %d\n", the_sequencer->index);
        index = sequencer_gpio_pico_execute(the_sequencer);
        sleep_ms(500);
    }
    free(the_sequencer);
    return 0;
}
