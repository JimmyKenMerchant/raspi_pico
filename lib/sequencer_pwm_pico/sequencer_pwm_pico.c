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

sequencer_pwm_pico* sequencer_pwm_pico_init(uchar8 slice, uint16* sequence) {
    sequencer_pwm_pico* sequencer_pwm = (sequencer_pwm_pico*)malloc(sizeof(sequencer_pwm_pico));
    if (sequencer_pwm == null) return sequencer_pwm;
    sequencer_pwm->slice = slice;
    sequencer_pwm->sequence = sequence;
    sequencer_pwm->sequence_length = sequencer_pwm_pico_get_sequence_length(sequencer_pwm->sequence);
    sequencer_pwm->index = 0;
    sequencer_pwm->sequence_interpolation = 0;
    sequencer_pwm->sequence_interpolation_accum = 0;
    return sequencer_pwm;
}

uint32 sequencer_pwm_pico_get_sequence_length(uint16* sequence) {
    for (uint32 i = 0; i < SEQUENCER_PWM_PICO_SEQUENCE_LENGTH_MAXIMUM; i++) {
        if (sequence[i] & 0x8000) {
            continue;
        } else {
            return i;
        }
    }
}

bool sequencer_pwm_pico_execute(sequencer_pwm_pico* sequencer_pwm) {
    if (sequencer_pwm == null) return EXIT_FAILURE;
    if (sequencer_pwm->index >= sequencer_pwm->sequence_length) sequencer_pwm->index = 0;
    if (sequencer_pwm->sequence_interpolation_accum) {
        sequencer_pwm->sequence_interpolation = _interpolate(sequencer_pwm->sequence_interpolation, sequencer_pwm->sequence[sequencer_pwm->index] & 0x7FFF, sequencer_pwm->sequence_interpolation_accum);
    } else {
        sequencer_pwm->sequence_interpolation = sequencer_pwm->sequence[sequencer_pwm->index] & 0x7FFF;
    }
    pwm_set_chan_level(sequencer_pwm->slice & 0x7F, (sequencer_pwm->slice & 0x80) >> 7, sequencer_pwm->sequence_interpolation);
    //printf("@sequencer_pwm_pico_execute 1 - sequencer_pwm->index: %d\n", sequencer_pwm->index);
    //printf("@sequencer_pwm_pico_execute 2 - sequencer_pwm->sequence[sequencer_pwm->index]: %d\n", sequencer_pwm->sequence[sequencer_pwm->index] & 0x7FFF);
    //printf("@sequencer_pwm_pico_execute 3 - sequencer_pwm->sequence_interpolation: %d\n", sequencer_pwm->sequence_interpolation & 0x7FFF);
    return EXIT_SUCCESS;
}
