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

#ifndef _UTIL_PEDAL_PICO_H
#define _UTIL_PEDAL_PICO_H 1

// Standards
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Dependancies
#include "pico/stdlib.h"
#include "pico/divider.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/resets.h"
#include "hardware/clocks.h"
#include "hardware/flash.h"
#include "hardware/regs/xip.h"
#include "hardware/structs/xip_ctrl.h"
// raspi_pico/include
#include "macros_pico.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions */
#ifndef UTIL_PEDAL_PICO_DEBUG
#define UTIL_PEDAL_PICO_DEBUG 0
#warning "UTIL_PEDAL_PICO_DEBUG is defined with the default value 0."
#endif
#ifndef UTIL_PEDAL_PICO_LED_1_GPIO
#define UTIL_PEDAL_PICO_LED_1_GPIO 25
#warning "UTIL_PEDAL_PICO_LED_1_GPIO is defined with the default value 25."
#endif
#ifndef UTIL_PEDAL_PICO_LED_2_GPIO
#define UTIL_PEDAL_PICO_LED_2_GPIO 8
#warning "UTIL_PEDAL_PICO_LED_2_GPIO is defined with the default value 8."
#endif
#ifndef UTIL_PEDAL_PICO_SW_1_GPIO
#define UTIL_PEDAL_PICO_SW_1_GPIO 10
#warning "UTIL_PEDAL_PICO_SW_1_GPIO is defined with the default value 10."
#endif
#ifndef UTIL_PEDAL_PICO_SW_2_GPIO
#define UTIL_PEDAL_PICO_SW_2_GPIO 9
#warning "UTIL_PEDAL_PICO_SW_2_GPIO is defined with the default value 9."
#endif
#ifndef UTIL_PEDAL_PICO_PWM_1_GPIO
#define UTIL_PEDAL_PICO_PWM_1_GPIO 16
#warning "UTIL_PEDAL_PICO_PWM_1_GPIO for Positve Terminal of Difference Amplifier is defined with the default value 16."
#endif
#ifndef UTIL_PEDAL_PICO_PWM_2_GPIO
#define UTIL_PEDAL_PICO_PWM_2_GPIO 17
#warning "UTIL_PEDAL_PICO_PWM_2_GPIO for Negative Terminal of Difference Amplifier is defined with the default value 17."
#endif
#ifndef UTIL_PEDAL_PICO_MULTI_BIT_0_GPIO
#define UTIL_PEDAL_PICO_MULTI_BIT_0_GPIO 11
#warning "UTIL_PEDAL_PICO_MULTI_BIT_0_GPIO is defined with the default value 11."
#endif
#ifndef UTIL_PEDAL_PICO_MULTI_BIT_1_GPIO
#define UTIL_PEDAL_PICO_MULTI_BIT_1_GPIO 15
#warning "UTIL_PEDAL_PICO_MULTI_BIT_1_GPIO is defined with the default value 15."
#endif
#ifndef UTIL_PEDAL_PICO_MULTI_BIT_2_GPIO
#define UTIL_PEDAL_PICO_MULTI_BIT_2_GPIO 12
#warning "UTIL_PEDAL_PICO_MULTI_BIT_2_GPIO is defined with the default value 12."
#endif
#ifndef UTIL_PEDAL_PICO_MULTI_BIT_3_GPIO
#define UTIL_PEDAL_PICO_MULTI_BIT_3_GPIO 14
#warning "UTIL_PEDAL_PICO_MULTI_BIT_3_GPIO is defined with the default value 14."
#endif
#define UTIL_PEDAL_PICO_XOSC 12000000 // Assuming Crystal Clock Speed
#define UTIL_PEDAL_PICO_TRANSIENT_RESPONSE 100000 // 100000 Micro Seconds, Pass through Transient Response of Power
#define UTIL_PEDAL_PICO_CORE_1_STACK_SIZE (1024 * 4) // 1024 Words, 4096 Bytes
#define UTIL_PEDAL_PICO_SW_THRESHOLD 30
#define UTIL_PEDAL_PICO_SW_SLEEP_TIME 1000
#define UTIL_PEDAL_PICO_PWM_IRQ_WRAP_PRIORITY 0xF0
#define UTIL_PEDAL_PICO_PWM_OFFSET 2048 // Ideal Middle Point
#define UTIL_PEDAL_PICO_PWM_PEAK 1023 // On the balanced monaural with a single power Op Amp, the offset of the positive is risen to OFFSET + (OFFSET / 2), and the offset of the negative is fallen to OFFSET - (OFFSET / 2).
#define UTIL_PEDAL_PICO_ADC_IRQ_FIFO_PRIORITY 0xFF
#define UTIL_PEDAL_PICO_ADC_0_GPIO 26
#define UTIL_PEDAL_PICO_ADC_1_GPIO 27
#define UTIL_PEDAL_PICO_ADC_2_GPIO 28
#define UTIL_PEDAL_PICO_ADC_ERROR_SINCE (uint64)1000000 // System Time (Micro Seconds) to Start Handling ADC Error
#define UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT 2048
#define UTIL_PEDAL_PICO_ADC_MIDDLE_MOVING_AVERAGE_NUMBER 16384 // Should be Power of 2 Because of Processing Speed (Logical Shift Left on Division)
#define UTIL_PEDAL_PICO_ADC_SHIFT 7 // 0x0-0xFFF (0-4095) to 0x0-0x1F (0-31)
#define UTIL_PEDAL_PICO_ADC_RESOLUTION 0x1F
#define UTIL_PEDAL_PICO_ADC_THRESHOLD 0x3F // Range is 0x0-0xFFF (0-4095) Divided by 0x80 (128) for 0x0-0x1F (0-31), (0x80 >> 1) - 1.
#define UTIL_PEDAL_PICO_OSC_SINE_1_TIME_MAX 9375
#define UTIL_PEDAL_PICO_MULTI_LENGTH 16 // 4-bit Length

/* Macros */
#define util_pedal_pico_cutoff_normalized(x, y) (_max(-y, _min(x, y))) // x: Value, y: Absolute Peak
#define util_pedal_pico_cutoff_biased(x, y, z) (_max(z, _min(x, y))) // x: Value, y: Peak, z: Bottom

/* Structs */
typedef struct {
    uchar8 pwm_1_slice;
    uchar8 pwm_1_channel;
    uchar8 pwm_2_slice;
    uchar8 pwm_2_channel;
    int32 output_1;
    int32 output_1_inverted;
} util_pedal_pico;

/* Global Variables */
volatile util_pedal_pico* util_pedal_pico_obj; // Pointer Needed to Be Initialized
volatile uint16 util_pedal_pico_on_adc_conversion_1;
volatile uint16 util_pedal_pico_on_adc_conversion_2;
volatile uint16 util_pedal_pico_on_adc_conversion_3;
volatile bool util_pedal_pico_on_adc_is_outstanding;
volatile uint16 util_pedal_pico_adc_middle_moving_average;
volatile uint32 util_pedal_pico_adc_middle_moving_average_sum;
volatile uchar8 util_pedal_pico_sw_gpio_1;
volatile uchar8 util_pedal_pico_sw_gpio_2;
volatile uchar8 util_pedal_pico_sw_mode;
volatile uint32 util_pedal_pico_debug_time;
volatile uchar8 util_pedal_pico_multi_gpio_bit_0;
volatile uchar8 util_pedal_pico_multi_gpio_bit_1;
volatile uchar8 util_pedal_pico_multi_gpio_bit_2;
volatile uchar8 util_pedal_pico_multi_gpio_bit_3;
volatile uchar8 util_pedal_pico_multi_mode;
volatile int32* util_pedal_pico_table_sine_1;
volatile int32* util_pedal_pico_table_pdf_1;
volatile int32* util_pedal_pico_table_pdf_2;
volatile int32* util_pedal_pico_table_pdf_3;
volatile int32* util_pedal_pico_table_log_1;
volatile int32* util_pedal_pico_table_log_2;
volatile int32* util_pedal_pico_table_power_1;

/* Hide Duplicate Declaration */
extern int32 util_pedal_pico_ex_table_sine_1[];
extern int32 util_pedal_pico_ex_table_pdf_1[];
extern int32 util_pedal_pico_ex_table_pdf_2[];
extern int32 util_pedal_pico_ex_table_pdf_3[];
extern int32 util_pedal_pico_ex_table_log_1[];
extern int32 util_pedal_pico_ex_table_log_2[];
extern int32 util_pedal_pico_ex_table_power_1[];

/* Functions */
void (*util_pedal_pico_process)(int32, uint16, uint16, uchar8); // Pointer Needed to Be Assigned
void util_pedal_pico_set_sys_clock_115200khz();
void util_pedal_pico_set_pwm_28125hz(pwm_config* ptr_config);
util_pedal_pico* util_pedal_pico_init(uchar8 gpio_1, uchar8 gpio_2);
void util_pedal_pico_init_adc();
void util_pedal_pico_start();
void util_pedal_pico_on_pwm_irq_wrap_handler();
void util_pedal_pico_renew_adc_middle_moving_average(uint16 conversion);
void util_pedal_pico_on_adc_irq_fifo();
void util_pedal_pico_init_sw(uchar8 gpio_1, uchar8 gpio_2);
void util_pedal_pico_free_sw(uchar8 gpio_1, uchar8 gpio_2);
void util_pedal_pico_sw_loop(uchar8 gpio_1, uchar8 gpio_2); // Three Point Switch
void util_pedal_pico_wait();
void util_pedal_pico_init_multi(uchar8 gpio_bit_0, uchar8 gpio_bit_1, uchar8 gpio_bit_2, uchar8 gpio_bit_3);
void util_pedal_pico_select_multi();
void (**util_pedal_pico_multi_set)();
void (**util_pedal_pico_multi_process)(uint16, uint16, uint16, uchar8);
void (**util_pedal_pico_multi_free)();
/**
 * Caution! These Functions tries to erase and write data to the on-board flash memory.
 * This program also turns off XIP, thus instruction code have to be stored at SRAM.
 */
#if PICO_COPY_TO_RAM
    void util_pedal_pico_xip_turn_off();
    void util_pedal_pico_flash_write(uint32 flash_offset, uchar8* buffer, uint32 size_in_byte); // size_in_byte Must Be Multiples of FLASH_SECTOR_SIZE (4096)
    void util_pedal_pico_flash_erase(uint32 flash_offset, uint32 size_in_byte); // size_in_byte Must Be Multiples of FLASH_SECTOR_SIZE (4096)
#else
    #warning "util_pedal_pico_xip_turn_off and util_pedal_pico_flash_write are not declared because PICO_COPY_TO_RAM is false."
#endif

#ifdef __cplusplus
}
#endif

#endif
