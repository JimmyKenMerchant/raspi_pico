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

#include "sequencer_gpio.h"

const unsigned long sequencer_gpio_list_length = 3;
const unsigned long sequencer_gpio_list[] = {15,16,25}; // Pin No. 0 to No. 29
// LSB in sequencer_gpio_command is the last in sequencer_gpio_list.
const unsigned short sequencer_gpio_command[] = {0b1000000000000011,
                                                 0b1000000000000101,
                                                 0b1000000000000011,
                                                 0b1000000000000101,
                                                 0b1000000000000000,
                                                 0b0000000000000000, // Clear MSB to Show End
                                                };
unsigned long sequencer_gpio_command_length;

unsigned long sequencer_gpio_init() {
    unsigned long gpio_mask = 0x00000000;
    for (unsigned char i = 0; i < sequencer_gpio_list_length; i++) {
        gpio_mask |= 0b1 << sequencer_gpio_list[i];
    }
    printf("@sequencer_gpio_init 1 - gpio_mask: %#x\n", gpio_mask);
    gpio_init_mask(gpio_mask);
    //gpio_set_dir_out_masked(gpio_mask);
    gpio_set_dir_masked(gpio_mask, 0xFFFFFFFF);
    printf("@sequencer_gpio_init 2 - GPIO-24 Direction: %d\n", gpio_is_dir_out(24));
    printf("@sequencer_gpio_init 3 - GPIO-25 Direction: %d\n", gpio_is_dir_out(25));
    for (unsigned long i = 0; i < SEQUENCER_GPIO_COMMAND_LENGTH_MAXIMUM; i++) {
        if (sequencer_gpio_command[i] & 0x8000) {
            continue;
        } else {
            sequencer_gpio_command_length = i;
            break;
        }
    }
    __asm__("dsb");
    __asm__("isb");
    return 0;
}

unsigned long sequencer_gpio_execute(unsigned long index) {
    if (index >= sequencer_gpio_command_length) index = 0;
    //printf("@sequencer_gpio_execute 1 - index: %d\n", index);
    //printf("@sequencer_gpio_execute 2 - sequencer_gpio_command_length: %d\n", sequencer_gpio_command_length);
    unsigned short command = sequencer_gpio_command[index];
    if (! (command & 0x8000)) return index;
    unsigned long gpio_mask_clear = 0x00000000;
    unsigned long gpio_mask_set = 0x00000000;
    for (unsigned char i = 0; i < sequencer_gpio_list_length; i++) {
        unsigned long id_gpio = sequencer_gpio_list[i];
        if (command & (0b1 << ((sequencer_gpio_list_length - i) - 1))) {
            gpio_mask_set |= 0b1 << id_gpio;
        } else {
            gpio_mask_clear |= 0b1 << id_gpio;
        }
    }
    //printf("@sequencer_gpio_execute 3 - gpio_mask_clear: %#x\n", gpio_mask_clear);
    //printf("@sequencer_gpio_execute 4 - gpio_mask_set: %#x\n", gpio_mask_set);
    gpio_put_masked(gpio_mask_clear, 0x00000000);
    gpio_put_masked(gpio_mask_set, 0xFFFFFFFF);
    index++;
    return index;
}

