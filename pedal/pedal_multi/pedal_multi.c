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
#include <stdlib.h>
#include <math.h>
// raspi_pico/include
#include "macros_pico.h"
#include "pedal_pico/pedal_pico_buffer.h"
#include "pedal_pico/pedal_pico_sideband.h"
#include "pedal_pico/pedal_pico_chorus.h"
#include "pedal_pico/pedal_pico_reverb.h"
#include "pedal_pico/pedal_pico_tape.h"
#include "pedal_pico/pedal_pico_phaser.h"
#include "pedal_pico/pedal_pico_planets.h"
#include "pedal_pico/pedal_pico_distortion.h"
#include "pedal_pico/pedal_pico_sustain.h"
#include "pedal_pico/pedal_pico_tremolo.h"
#include "pedal_pico/pedal_pico_envelope.h"
#include "pedal_pico/pedal_pico_looper.h"
#include "util_pedal_pico_ex.h"

#define PEDAL_MULTI_SLEEP_TIME 250000 // 250000 Micro Seconds

/* Definitions for Combinations */
#define PEDAL_MULTI_MEMORY_MAX 12
#define PEDAL_MULTI_MEMORY_ROOMREVERB_INDEX 0
#define PEDAL_MULTI_MEMORY_PLANETSREVERB_INDEX 4
#define PEDAL_MULTI_MEMORY_RACKPHASER_INDEX 8

/* Global Variables for Combinations */
volatile int16_t* pedal_multi_memory_buffer;
bool pedal_multi_memory_is_pending_write;
uint8_t pedal_multi_previous_sw_mode;
uint16_t pedal_multi_roomreverb_reverb_conversion_2;
uint16_t pedal_multi_roomreverb_reverb_conversion_3;
uint16_t pedal_multi_roomreverb_chorus_conversion_2;
uint16_t pedal_multi_roomreverb_chorus_conversion_3;
uint16_t pedal_multi_planetsreverb_planets_conversion_2;
uint16_t pedal_multi_planetsreverb_planets_conversion_3;
uint16_t pedal_multi_planetsreverb_reverb_conversion_2;
uint16_t pedal_multi_planetsreverb_reverb_conversion_3;
uint16_t pedal_multi_rackphaser_envelope_conversion_2;
uint16_t pedal_multi_rackphaser_envelope_conversion_3;
uint16_t pedal_multi_rackphaser_phaser_conversion_2;
uint16_t pedal_multi_rackphaser_phaser_conversion_3;

/* Functions for Combinations */
void pedal_multi_roomreverb_set();
void pedal_multi_planetsreverb_set();
void pedal_multi_distreverb_set();
void pedal_multi_fuzzplanets_set();
void pedal_multi_parallelreverb_set();
void pedal_multi_distsustain_set();
void pedal_multi_truebuffer_set();
void pedal_multi_rackphaser_set();
void pedal_multi_roomreverb_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_multi_planetsreverb_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_multi_distreverb_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_multi_fuzzplanets_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_multi_parallelreverb_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_multi_distsustain_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_multi_truebuffer_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_multi_rackphaser_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode);
void pedal_multi_roomreverb_free();
void pedal_multi_planetsreverb_free();
void pedal_multi_distreverb_free();
void pedal_multi_fuzzplanets_free();
void pedal_multi_parallelreverb_free();
void pedal_multi_distsustain_free();
void pedal_multi_truebuffer_free();
void pedal_multi_rackphaser_free();

int main(void) {
    util_pedal_pico_set_sys_clock_115200khz();
    sleep_us(UTIL_PEDAL_PICO_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(UTIL_PEDAL_PICO_LED_1_GPIO);
    gpio_set_dir(UTIL_PEDAL_PICO_LED_1_GPIO, GPIO_OUT);
    gpio_put(UTIL_PEDAL_PICO_LED_1_GPIO, 1);
    /* Initialize PWM */
    #if UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX != UTIL_PEDAL_PICO_EX_OSC_SINE_TIME_MAX
        #error "UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX isn't eqaul to UTIL_PEDAL_PICO_EX_OSC_SINE_TIME_MAX. Include util_pedal_pico_ex.h?"
    #endif
    #if UTIL_PEDAL_PICO_OSC_TRIANGLE_1_TIME_MAX != UTIL_PEDAL_PICO_EX_OSC_TRIANGLE_TIME_MAX
        #error "UTIL_PEDAL_PICO_OSC_TRIANGLE_1_TIME_MAX isn't eqaul to UTIL_PEDAL_PICO_EX_OSC_TRIANGLE_TIME_MAX. Include util_pedal_pico_ex.h?"
    #endif
    #if UTIL_PEDAL_PICO_PWM_PEAK != UTIL_PEDAL_PICO_EX_PEAK
        #error "UTIL_PEDAL_PICO_PWM_PEAK isn't eqaul to UTIL_PEDAL_PICO_EX_PEAK. Include util_pedal_pico_ex.h?"
    #endif
    pedal_pico_buffer = util_pedal_pico_init(UTIL_PEDAL_PICO_PWM_1_GPIO, UTIL_PEDAL_PICO_PWM_2_GPIO);
    pedal_pico_sideband = util_pedal_pico_obj;
    pedal_pico_chorus = util_pedal_pico_obj;
    pedal_pico_reverb = util_pedal_pico_obj;
    pedal_pico_tape = util_pedal_pico_obj;
    pedal_pico_phaser = util_pedal_pico_obj;
    pedal_pico_planets = util_pedal_pico_obj;
    pedal_pico_distortion = util_pedal_pico_obj;
    pedal_pico_sustain = util_pedal_pico_obj;
    pedal_pico_tremolo = util_pedal_pico_obj;
    pedal_pico_envelope = util_pedal_pico_obj;
    /* Initialize ADC */
    util_pedal_pico_init_adc();
    /* Initialize Multi Functions */
    util_pedal_pico_init_multi(UTIL_PEDAL_PICO_MULTI_BIT_0_GPIO, UTIL_PEDAL_PICO_MULTI_BIT_1_GPIO, UTIL_PEDAL_PICO_MULTI_BIT_2_GPIO, UTIL_PEDAL_PICO_MULTI_BIT_3_GPIO, UTIL_PEDAL_PICO_LED_2_MULTI_BIT_4_GPIO);
    /* Set Individual Functions to Multi Functions */
    util_pedal_pico_multi_set[0] = pedal_pico_buffer_set;
    util_pedal_pico_multi_set[1] = pedal_pico_sideband_set;
    util_pedal_pico_multi_set[2] = pedal_pico_chorus_set;
    util_pedal_pico_multi_set[3] = pedal_pico_reverb_set;
    util_pedal_pico_multi_set[4] = pedal_multi_roomreverb_set;
    util_pedal_pico_multi_set[5] = pedal_multi_parallelreverb_set;
    util_pedal_pico_multi_set[6] = pedal_pico_tape_set;
    util_pedal_pico_multi_set[7] = pedal_pico_phaser_set;
    util_pedal_pico_multi_set[8] = pedal_multi_rackphaser_set;
    util_pedal_pico_multi_set[9] = pedal_pico_planets_set;
    util_pedal_pico_multi_set[10] = pedal_multi_planetsreverb_set;
    util_pedal_pico_multi_set[11] = pedal_pico_tremolo_set;
    util_pedal_pico_multi_set[12] = pedal_pico_envelope_set;
    util_pedal_pico_multi_set[13] = pedal_pico_distortion_set;
    util_pedal_pico_multi_set[14] = pedal_pico_sustain_set;
    util_pedal_pico_multi_set[15] = pedal_multi_distsustain_set;
    util_pedal_pico_multi_set[16] = pedal_multi_truebuffer_set;
    util_pedal_pico_multi_process[0] = pedal_pico_buffer_process;
    util_pedal_pico_multi_process[1] = pedal_pico_sideband_process;
    util_pedal_pico_multi_process[2] = pedal_pico_chorus_process;
    util_pedal_pico_multi_process[3] = pedal_pico_reverb_process;
    util_pedal_pico_multi_process[4] = pedal_multi_roomreverb_process;
    util_pedal_pico_multi_process[5] = pedal_multi_parallelreverb_process;
    util_pedal_pico_multi_process[6] = pedal_pico_tape_process;
    util_pedal_pico_multi_process[7] = pedal_pico_phaser_process;
    util_pedal_pico_multi_process[8] = pedal_multi_rackphaser_process;
    util_pedal_pico_multi_process[9] = pedal_pico_planets_process;
    util_pedal_pico_multi_process[10] = pedal_multi_planetsreverb_process;
    util_pedal_pico_multi_process[11] = pedal_pico_tremolo_process;
    util_pedal_pico_multi_process[12] = pedal_pico_envelope_process;
    util_pedal_pico_multi_process[13] = pedal_pico_distortion_process;
    util_pedal_pico_multi_process[14] = pedal_pico_sustain_process;
    util_pedal_pico_multi_process[15] = pedal_multi_distsustain_process;
    util_pedal_pico_multi_process[16] = pedal_multi_truebuffer_process;
    util_pedal_pico_multi_free[0] = pedal_pico_buffer_free;
    util_pedal_pico_multi_free[1] = pedal_pico_sideband_free;
    util_pedal_pico_multi_free[2] = pedal_pico_chorus_free;
    util_pedal_pico_multi_free[3] = pedal_pico_reverb_free;
    util_pedal_pico_multi_free[4] = pedal_multi_roomreverb_free;
    util_pedal_pico_multi_free[5] = pedal_multi_parallelreverb_free;
    util_pedal_pico_multi_free[6] = pedal_pico_tape_free;
    util_pedal_pico_multi_free[7] = pedal_pico_phaser_free;
    util_pedal_pico_multi_free[8] = pedal_multi_rackphaser_free;
    util_pedal_pico_multi_free[9] = pedal_pico_planets_free;
    util_pedal_pico_multi_free[10] = pedal_multi_planetsreverb_free;
    util_pedal_pico_multi_free[11] = pedal_pico_tremolo_free;
    util_pedal_pico_multi_free[12] = pedal_pico_envelope_free;
    util_pedal_pico_multi_free[13] = pedal_pico_distortion_free;
    util_pedal_pico_multi_free[14] = pedal_pico_sustain_free;
    util_pedal_pico_multi_free[15] = pedal_multi_distsustain_free;
    util_pedal_pico_multi_free[16] = pedal_multi_truebuffer_free;
    /* Initialize Memory */
    pedal_multi_memory_buffer = (uint16_t*)calloc(FLASH_SECTOR_SIZE >> 1, sizeof(uint16_t));
    if (pedal_pico_looper_flash_reserve[FLASH_SECTOR_SIZE - 1] == 0x88) { // Check the last to know whether flash memory is initialized or not.
        for(uint16_t i = 0; i < PEDAL_MULTI_MEMORY_MAX; i++) {
            pedal_multi_memory_buffer[i] = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
        }
        util_pedal_pico_flash_write((uint32_t)pedal_pico_looper_flash_reserve - XIP_BASE, (uint8_t*)pedal_multi_memory_buffer, FLASH_SECTOR_SIZE);
    } else {
        for(uint16_t i = 0; i < PEDAL_MULTI_MEMORY_MAX; i++) {
            pedal_multi_memory_buffer[i] = ((uint16_t)pedal_pico_looper_flash_reserve[i * 2] | ((uint16_t)pedal_pico_looper_flash_reserve[(i * 2) + 1]) << 8) & UTIL_PEDAL_PICO_ADC_RANGE; // Prevent Unlikely Values in Case
        }
    }
    pedal_multi_memory_is_pending_write = false;
    pedal_multi_previous_sw_mode = 0;
    /* Initialize Switch */
    util_pedal_pico_init_sw(UTIL_PEDAL_PICO_SW_1_GPIO, UTIL_PEDAL_PICO_SW_2_GPIO);
    /* Unique Variables and Functions */
    util_pedal_pico_select_multi();
    /* Launch Core 1 */
    uint32_t* stack_pointer = (int32_t*)malloc(UTIL_PEDAL_PICO_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(util_pedal_pico_start, stack_pointer, UTIL_PEDAL_PICO_CORE_1_STACK_SIZE);
    while (true) {
        util_pedal_pico_select_multi();
        if(pedal_multi_memory_is_pending_write) {
            pedal_multi_memory_is_pending_write = false;
            util_pedal_pico_flash_write((uint32_t)pedal_pico_looper_flash_reserve - XIP_BASE, (uint8_t*)pedal_multi_memory_buffer, FLASH_SECTOR_SIZE);
        }
        sleep_us(PEDAL_MULTI_SLEEP_TIME);
    }
    return 0;
}

void pedal_multi_roomreverb_set() {
    pedal_multi_roomreverb_reverb_conversion_2 = pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_ROOMREVERB_INDEX];
    pedal_multi_roomreverb_reverb_conversion_3 = pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_ROOMREVERB_INDEX + 1];
    pedal_multi_roomreverb_chorus_conversion_2 = pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_ROOMREVERB_INDEX + 2];
    pedal_multi_roomreverb_chorus_conversion_3 = pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_ROOMREVERB_INDEX + 3];
    pedal_pico_reverb_set();
    pedal_pico_chorus_set();
}

void pedal_multi_planetsreverb_set() {
    pedal_multi_planetsreverb_planets_conversion_2 = pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_PLANETSREVERB_INDEX];
    pedal_multi_planetsreverb_planets_conversion_3 = pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_PLANETSREVERB_INDEX + 1];
    pedal_multi_planetsreverb_reverb_conversion_2 = pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_PLANETSREVERB_INDEX + 2];
    pedal_multi_planetsreverb_reverb_conversion_3 = pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_PLANETSREVERB_INDEX + 3];
    pedal_pico_planets_set();
    pedal_pico_reverb_set();
}

void pedal_multi_distreverb_set() {
    pedal_pico_distortion_set();
    pedal_pico_reverb_set();
}

void pedal_multi_fuzzplanets_set() {
    pedal_pico_distortion_set();
    pedal_pico_planets_set();
}

void pedal_multi_parallelreverb_set() {
    pedal_pico_reverb_set();
}

void pedal_multi_distsustain_set() {
    pedal_pico_distortion_set();
    pedal_pico_sustain_set();
}

void pedal_multi_truebuffer_set() {
    pedal_pico_buffer_set();
}

void pedal_multi_rackphaser_set() {
    pedal_multi_rackphaser_envelope_conversion_2 = pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_RACKPHASER_INDEX];
    pedal_multi_rackphaser_envelope_conversion_3 = pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_RACKPHASER_INDEX + 1];
    pedal_multi_rackphaser_phaser_conversion_2 = pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_RACKPHASER_INDEX + 2];
    pedal_multi_rackphaser_phaser_conversion_3 = pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_RACKPHASER_INDEX + 3];
    pedal_pico_envelope_set();
    pedal_pico_phaser_set();
}

void pedal_multi_roomreverb_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (sw_mode == 1) {
        pedal_multi_roomreverb_reverb_conversion_2 = conversion_2;
        pedal_multi_roomreverb_reverb_conversion_3 = conversion_3;
    } else if (sw_mode == 2) {
        pedal_multi_roomreverb_chorus_conversion_2 = conversion_2;
        pedal_multi_roomreverb_chorus_conversion_3 = conversion_3;
    } else {
        if (pedal_multi_previous_sw_mode == 1) {
            pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_ROOMREVERB_INDEX] = conversion_2;
            pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_ROOMREVERB_INDEX + 1] = conversion_3;
            pedal_multi_memory_is_pending_write = true;
        } else if (pedal_multi_previous_sw_mode == 2) {
            pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_ROOMREVERB_INDEX + 2] = conversion_2;
            pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_ROOMREVERB_INDEX + 3] = conversion_3;
            pedal_multi_memory_is_pending_write = true;
        }
    }
    pedal_multi_previous_sw_mode = sw_mode;
    /* Objective entities, util_pedal_pico_obj, pedal_pico_reverb, and pedal_pico_chorus point the same struct and memory space */
    pedal_pico_reverb_process(normalized_1, pedal_multi_roomreverb_reverb_conversion_2, pedal_multi_roomreverb_reverb_conversion_3, 0);
    pedal_pico_chorus_process(util_pedal_pico_obj->output_1, pedal_multi_roomreverb_chorus_conversion_2, pedal_multi_roomreverb_chorus_conversion_3, 0);
}

void pedal_multi_planetsreverb_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (sw_mode == 1) {
        pedal_multi_planetsreverb_planets_conversion_2 = conversion_2;
        pedal_multi_planetsreverb_planets_conversion_3 = conversion_3;
    } else if (sw_mode == 2) {
        pedal_multi_planetsreverb_reverb_conversion_2 = conversion_2;
        pedal_multi_planetsreverb_reverb_conversion_3 = conversion_3;
    } else {
        if (pedal_multi_previous_sw_mode == 1) {
            pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_PLANETSREVERB_INDEX] = conversion_2;
            pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_PLANETSREVERB_INDEX + 1] = conversion_3;
            pedal_multi_memory_is_pending_write = true;
        } else if (pedal_multi_previous_sw_mode == 2) {
            pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_PLANETSREVERB_INDEX + 2] = conversion_2;
            pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_PLANETSREVERB_INDEX + 3] = conversion_3;
            pedal_multi_memory_is_pending_write = true;
        }
    }
    pedal_multi_previous_sw_mode = sw_mode;
    /* Objective entities, util_pedal_pico_obj, pedal_pico_planets, and pedal_pico_reverb point the same struct and memory space */
    pedal_pico_planets_process(normalized_1, pedal_multi_planetsreverb_planets_conversion_2, pedal_multi_planetsreverb_planets_conversion_3, 0);
    pedal_pico_reverb_process(util_pedal_pico_obj->output_1, pedal_multi_planetsreverb_reverb_conversion_2, pedal_multi_planetsreverb_reverb_conversion_3, 0);
}

void pedal_multi_distreverb_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    /* Objective entities, util_pedal_pico_obj, pedal_pico_distortion, and pedal_pico_reverb point the same struct and memory space */
    pedal_pico_distortion_process(normalized_1, 0, 0, 2);
    pedal_pico_reverb_process(util_pedal_pico_obj->output_1, conversion_2, conversion_3, sw_mode);
}

void pedal_multi_fuzzplanets_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    /* Objective entities, util_pedal_pico_obj, pedal_pico_distortion, and pedal_pico_planets point the same struct and memory space */
    pedal_pico_distortion_process(normalized_1, 0, 0, 1);
    pedal_pico_planets_process(util_pedal_pico_obj->output_1, conversion_2, conversion_3, sw_mode);
}

void pedal_multi_parallelreverb_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    /* Objective entities, util_pedal_pico_obj, pedal_pico_sideband, and pedal_pico_reverb point the same struct and memory space */
    pedal_pico_reverb_process(normalized_1, conversion_2, conversion_3, sw_mode);
    normalized_1 >>= 1;
    util_pedal_pico_obj->output_1 += normalized_1;
    util_pedal_pico_obj->output_1_inverted -= normalized_1;
}

void pedal_multi_distsustain_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    /* Objective entities, util_pedal_pico_obj, pedal_pico_distortion, and pedal_pico_sustain point the same struct and memory space */
    pedal_pico_distortion_process(normalized_1, 0, 0, 2);
    pedal_pico_sustain_process(util_pedal_pico_obj->output_1, conversion_2, conversion_3, sw_mode);
}

void pedal_multi_truebuffer_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    /* Objective entities, util_pedal_pico_obj, pedal_pico_buffer point the same struct and memory space */
    pedal_pico_buffer_process(normalized_1, 0, 0, 0);
}

void pedal_multi_rackphaser_process(int32_t normalized_1, uint16_t conversion_2, uint16_t conversion_3, uint8_t sw_mode) {
    if (sw_mode == 1) {
        pedal_multi_rackphaser_envelope_conversion_2 = conversion_2;
        pedal_multi_rackphaser_envelope_conversion_3 = conversion_3;
    } else if (sw_mode == 2) {
        pedal_multi_rackphaser_phaser_conversion_2 = conversion_2;
        pedal_multi_rackphaser_phaser_conversion_3 = conversion_3;
    } else {
        if (pedal_multi_previous_sw_mode == 1) {
            pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_RACKPHASER_INDEX] = conversion_2;
            pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_RACKPHASER_INDEX + 1] = conversion_3;
            pedal_multi_memory_is_pending_write = true;
        } else if (pedal_multi_previous_sw_mode == 2) {
            pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_RACKPHASER_INDEX + 2] = conversion_2;
            pedal_multi_memory_buffer[PEDAL_MULTI_MEMORY_RACKPHASER_INDEX + 3] = conversion_3;
            pedal_multi_memory_is_pending_write = true;
        }
    }
    pedal_multi_previous_sw_mode = sw_mode;
    /* Objective entities, util_pedal_pico_obj, pedal_pico_envelope, and pedal_pico_phaser point the same struct and memory space */
    pedal_pico_envelope_process(normalized_1, pedal_multi_rackphaser_envelope_conversion_2, pedal_multi_rackphaser_envelope_conversion_3, 2);
    pedal_pico_phaser_process(util_pedal_pico_obj->output_1, pedal_multi_rackphaser_phaser_conversion_2, pedal_multi_rackphaser_phaser_conversion_3, 2);
}

void pedal_multi_roomreverb_free() {
    pedal_pico_reverb_free();
    pedal_pico_chorus_free();
}

void pedal_multi_planetsreverb_free() {
    pedal_pico_planets_free();
    pedal_pico_reverb_free();
}

void pedal_multi_distreverb_free() {
    pedal_pico_distortion_free();
    pedal_pico_reverb_free();
}

void pedal_multi_fuzzplanets_free() {
    pedal_pico_distortion_free();
    pedal_pico_planets_free();
}

void pedal_multi_parallelreverb_free() {
    pedal_pico_reverb_free();
}

void pedal_multi_distsustain_free() {
    pedal_pico_distortion_free();
    pedal_pico_sustain_free();
}

void pedal_multi_truebuffer_free() {
    pedal_pico_buffer_free();
}

void pedal_multi_rackphaser_free() {
    pedal_pico_envelope_free();
    pedal_pico_phaser_free();
}
