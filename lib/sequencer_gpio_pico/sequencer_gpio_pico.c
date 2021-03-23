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

#include "sequencer_gpio_pico.h"

const uint32 sequencer_gpio_pico_pinlist_length = 3;
const uchar8 sequencer_gpio_pico_pinlist[] = {15,16,25}; // Pin No. 0 to No. 29
// LSB in sequencer_gpio_pico_sequence is the last in sequencer_gpio_pico_pinlist.
const uint16 sequencer_gpio_pico_sequence[] = {0b1000000000000010,
                                               0b1000000000000101,
                                               0b1000000000000010,
                                               0b1000000000000101,
                                               0b1000000000000010,
                                               0b1000000000000101,
                                               0b0000000000000000, // Clear MSB to Show End
                                              };
uint32 sequencer_gpio_pico_sequence_length;

uint32 sequencer_gpio_pico_init() {
    uint32 gpio_mask = 0x00000000;
    for (uchar8 i = 0; i < sequencer_gpio_pico_pinlist_length; i++) {
        gpio_mask |= 0b1 << sequencer_gpio_pico_pinlist[i];
    }
    printf("@sequencer_gpio_pico_init 1 - gpio_mask: %#x\n", gpio_mask);
    gpio_init_mask(gpio_mask);
    //gpio_set_dir_out_masked(gpio_mask);
    gpio_set_dir_masked(gpio_mask, 0xFFFFFFFF);
    printf("@sequencer_gpio_pico_init 2 - GPIO-24 Direction: %d\n", gpio_is_dir_out(24));
    printf("@sequencer_gpio_pico_init 3 - GPIO-25 Direction: %d\n", gpio_is_dir_out(25));
    for (uint32 i = 0; i < SEQUENCER_GPIO_PICO_SEQUENCE_LENGTH_MAXIMUM; i++) {
        if (sequencer_gpio_pico_sequence[i] & 0x8000) {
            continue;
        } else {
            sequencer_gpio_pico_sequence_length = i;
            break;
        }
    }
    __asm__("dsb");
    __asm__("isb");
    return 0;
}

uint32 sequencer_gpio_pico_execute(uint32 index) {
    if (index >= sequencer_gpio_pico_sequence_length) index = 0;
    //printf("@sequencer_gpio_pico_execute 1 - index: %d\n", index);
    //printf("@sequencer_gpio_pico_execute 2 - sequencer_gpio_pico_sequence_length: %d\n", sequencer_gpio_pico_sequence_length);
    uint16 command = sequencer_gpio_pico_sequence[index];
    if (! (command & 0x8000)) return index;
    uint32 gpio_mask_clear = 0x00000000;
    uint32 gpio_mask_set = 0x00000000;
    for (uchar8 i = 0; i < sequencer_gpio_pico_pinlist_length; i++) {
        uint32 id_gpio = sequencer_gpio_pico_pinlist[i];
        if (command & (0b1 << ((sequencer_gpio_pico_pinlist_length - i) - 1))) {
            gpio_mask_set |= 0b1 << id_gpio;
        } else {
            gpio_mask_clear |= 0b1 << id_gpio;
        }
    }
    //printf("@sequencer_gpio_pico_execute 3 - gpio_mask_clear: %#x\n", gpio_mask_clear);
    //printf("@sequencer_gpio_pico_execute 4 - gpio_mask_set: %#x\n", gpio_mask_set);
    gpio_put_masked(gpio_mask_clear, 0x00000000);
    gpio_put_masked(gpio_mask_set, 0xFFFFFFFF);
    index++;
    return index;
}

