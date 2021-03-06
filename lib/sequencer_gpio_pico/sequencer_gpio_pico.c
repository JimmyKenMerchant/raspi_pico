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

sequencer_gpio_pico* sequencer_gpio_pico_init(uint8_t* pinlist, uint32_t pinlist_length, uint16_t* sequence) {
    sequencer_gpio_pico* sequencer_gpio = (sequencer_gpio_pico*)malloc(sizeof(sequencer_gpio_pico));
    if (sequencer_gpio == null) return sequencer_gpio;
    sequencer_gpio->pinlist = pinlist;
    sequencer_gpio->pinlist_length = pinlist_length;
    sequencer_gpio->sequence = sequence;
    uint32_t gpio_mask = 0x00000000;
    for (uint8_t i = 0; i < sequencer_gpio->pinlist_length; i++) {
        gpio_mask |= 0b1 << sequencer_gpio->pinlist[i];
    }
    //printf("@sequencer_gpio_pico_init 1 - gpio_mask: %#x\n", gpio_mask);
    gpio_init_mask(gpio_mask);
    //gpio_set_dir_out_masked(gpio_mask);
    gpio_set_dir_masked(gpio_mask, 0xFFFFFFFF);
    //printf("@sequencer_gpio_pico_init 2 - GPIO-24 Direction: %d\n", gpio_is_dir_out(24));
    //printf("@sequencer_gpio_pico_init 3 - GPIO-25 Direction: %d\n", gpio_is_dir_out(25));
    sequencer_gpio->sequence_length = sequencer_gpio_pico_get_sequence_length(sequencer_gpio->sequence);
    sequencer_gpio->index = 0;
    __dsb();
    __isb();
    return sequencer_gpio;
}

uint32_t sequencer_gpio_pico_get_sequence_length(uint16_t* sequence) {
    for (uint32_t i = 0; i < SEQUENCER_GPIO_PICO_SEQUENCE_LENGTH_MAXIMUM; i++) {
        if (sequence[i] & 0x8000) {
            continue;
        } else {
            return i;
        }
    }
}

bool sequencer_gpio_pico_execute(sequencer_gpio_pico* sequencer_gpio) {
    if (sequencer_gpio == null) return EXIT_FAILURE;
    if (sequencer_gpio->index >= sequencer_gpio->sequence_length) sequencer_gpio->index = 0;
    //printf("@sequencer_gpio_pico_execute 1 - sequencer_gpio->index: %d\n", sequencer_gpio->index);
    //printf("@sequencer_gpio_pico_execute 2 - sequencer_gpio->sequence_length: %d\n", sequencer_gpio->sequence_length);
    uint16_t command = sequencer_gpio->sequence[sequencer_gpio->index];
    if (! (command & 0x8000)) return EXIT_FAILURE;
    uint32_t gpio_mask_clear = 0x00000000;
    uint32_t gpio_mask_set = 0x00000000;
    for (uint8_t i = 0; i < sequencer_gpio->pinlist_length; i++) {
        uint32_t id_gpio = sequencer_gpio->pinlist[i];
        if (command & (0b1 << ((sequencer_gpio->pinlist_length - i) - 1))) {
            gpio_mask_set |= 0b1 << id_gpio;
        } else {
            gpio_mask_clear |= 0b1 << id_gpio;
        }
    }
    //printf("@sequencer_gpio_pico_execute 3 - gpio_mask_clear: %#x\n", gpio_mask_clear);
    //printf("@sequencer_gpio_pico_execute 4 - gpio_mask_set: %#x\n", gpio_mask_set);
    gpio_put_masked(gpio_mask_clear, 0x00000000);
    gpio_put_masked(gpio_mask_set, 0xFFFFFFFF);
    sequencer_gpio->index++;
    return EXIT_SUCCESS;
}
