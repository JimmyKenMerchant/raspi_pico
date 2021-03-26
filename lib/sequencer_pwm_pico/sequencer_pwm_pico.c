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

#include "sequencer_pwm_pico.h"

sequencer_pwm_pico* sequencer_pwm_pico_init(uchar8* slicelist, uint32 slicelist_length, uint16** sequence) {
    sequencer_pwm_pico* sequencer_pwm = (sequencer_pwm_pico*)malloc(sizeof(sequencer_pwm_pico));
    if (sequencer_pwm == null) return sequencer_pwm;
    sequencer_pwm->slicelist = slicelist;
    sequencer_pwm->slicelist_length = slicelist_length;
    sequencer_pwm->sequence = sequence;
    sequencer_pwm->sequence_length = sequencer_pwm_pico_get_sequence_length(sequencer_pwm->sequence);
    return sequencer_pwm;
}

uint32 sequencer_pwm_pico_get_sequence_length(uint16** sequence) {
    for (uint32 i = 0; i < SEQUENCER_PWM_PICO_SEQUENCE_LENGTH_MAXIMUM; i++) {
        if (sequence[0][i] & 0x8000) {
            continue;
        } else {
            return i;
        }
    }
}

bool sequencer_pwm_pico_execute(sequencer_pwm_pico* sequencer_pwm) {
    if (sequencer_pwm == null) return EXIT_FAILURE;
    if (sequencer_pwm->index >= sequencer_pwm->sequence_length) sequencer_pwm->index = 0;
    //printf("@sequencer_pwm_pico_execute 1 - sequencer_pwm->index: %d\n", sequencer_pwm->index);
    //printf("@sequencer_pwm_pico_execute 2 - sequencer_pwm->sequence_length: %d\n", sequencer_pwm->sequence_length);
    for (uchar8 i = 0; i < sequencer_pwm->slicelist_length; i++) {
        uint32 id_slice = sequencer_pwm->slicelist[i];
        uint16 command = sequencer_pwm->sequence[i][sequencer_pwm->index];
        pwm_set_chan_level(id_slice & 0x7F, id_slice & 0x80, command & 0x7FFFF);
    }
    //printf("@sequencer_pwm_pico_execute 3 - gpio_mask_clear: %#x\n", gpio_mask_clear);
    //printf("@sequencer_pwm_pico_execute 4 - gpio_mask_set: %#x\n", gpio_mask_set);
    sequencer_pwm->index++;
    return EXIT_SUCCESS;
}
