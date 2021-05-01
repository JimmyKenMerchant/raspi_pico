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
// Dependancies
#include "pico/stdlib.h"
#include "pico/divider.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/resets.h"
// raspi_pico/include
#include "macros_pico.h"
#include "util_pedal_pico.h"
#include "util_pedal_pico_ex.h"
// Private header
//#include "pedal_phaser.h"

#define PEDAL_PHASER_TRANSIENT_RESPONSE 100000 // 100000 Micro Seconds
#define PEDAL_PHASER_CORE_1_STACK_SIZE 1024 * 4 // 1024 Words, 4096 Bytes
#define PEDAL_PHASER_LED_GPIO 25
#define PEDAL_PHASER_SW_1_GPIO 14
#define PEDAL_PHASER_SW_2_GPIO 15
#define PEDAL_PHASER_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_PHASER_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_PHASER_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_PHASER_PWM_PEAK 2047
#define PEDAL_PHASER_GAIN 1
#define PEDAL_PHASER_COEFFICIENT_SWING_PEAK_FIXED_1 (int32)(0x00010000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PHASER_DELAY_TIME_MAX 2049 // Don't Use Delay Time = 0
#define PEDAL_PHASER_DELAY_TIME_FIXED_1 2048 // 28125 Divided by 2024 (13.90Hz, Folding Frequency is 6.95Hz)
#define PEDAL_PHASER_DELAY_TIME_FIXED_2 256 // 28125 Divided by 256 (109.86Hz, Folding Frequency is 59.93Hz)
#define PEDAL_PHASER_DELAY_TIME_FIXED_3 64 // 28125 Divided by 64 (439.45Hz, Folding Frequency is 219.73Hz)
#define PEDAL_PHASER_OSC_SINE_1_TIME_MAX 9375
#define PEDAL_PHASER_OSC_SINE_1_TIME_MULTIPLIER 6
#define PEDAL_PHASER_OSC_START_THRESHOLD_MULTIPLIER 1 // From -66.22dB (Loss 2047) to -36.39dB (Loss 66) in ADC_VREF (Typically 3.3V)
#define PEDAL_PHASER_OSC_START_COUNT_MAX 2000 // 28125 Divided by 2000 = Approx. 14Hz

volatile util_pedal_pico* pedal_phaser;
volatile uint16 pedal_phaser_conversion_1;
volatile uint16 pedal_phaser_conversion_2;
volatile uint16 pedal_phaser_conversion_3;
volatile int32 pedal_phaser_coefficient_swing;
volatile int16* pedal_phaser_delay_x_1;
volatile int16* pedal_phaser_delay_y_1;
volatile int16* pedal_phaser_delay_x_2;
volatile int16* pedal_phaser_delay_y_2;
volatile uint16 pedal_phaser_delay_time;
volatile uint16 pedal_phaser_delay_index;
volatile uint32 pedal_phaser_osc_sine_1_index;
volatile uint16 pedal_phaser_osc_speed;
volatile char8 pedal_phaser_osc_start_threshold;
volatile uint16 pedal_phaser_osc_start_count;
volatile uint32 pedal_phaser_debug_time;

void pedal_phaser_core_1();
void pedal_phaser_set();
void pedal_phaser_on_pwm_irq_wrap();
void pedal_phaser_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3);
void pedal_phaser_free();

int main(void) {
    //stdio_init_all();
    util_pedal_pico_set_sys_clock_115200khz();
    //stdio_init_all(); // Re-init for UART Baud Rate
    sleep_us(PEDAL_PHASER_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(PEDAL_PHASER_LED_GPIO);
    gpio_set_dir(PEDAL_PHASER_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_PHASER_LED_GPIO, true);
    uint32* stack_pointer = (int32*)malloc(PEDAL_PHASER_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(pedal_phaser_core_1, stack_pointer, PEDAL_PHASER_CORE_1_STACK_SIZE);
    //pedal_phaser_debug_time = 0;
    //uint32 from_time = time_us_32();
    //printf("@main 1 - Let's Start!\n");
    //pedal_phaser_debug_time = time_us_32() - from_time;
    //printf("@main 2 - pedal_phaser_debug_time %d\n", pedal_phaser_debug_time);
    while (true) {
        //printf("@main 3 - pedal_phaser_conversion_1 %0x\n", pedal_phaser_conversion_1);
        //printf("@main 4 - pedal_phaser_conversion_2 %0x\n", pedal_phaser_conversion_2);
        //printf("@main 5 - pedal_phaser_conversion_3 %0x\n", pedal_phaser_conversion_3);
        //printf("@main 6 - multicore_fifo_pop_blocking() %d\n", multicore_fifo_pop_blocking());
        //printf("@main 7 - pedal_phaser_debug_time %d\n", pedal_phaser_debug_time);
        //sleep_ms(500);
        //tight_loop_contents();
        __wfi();
    }
    return 0;
}

void pedal_phaser_core_1() {
    /* PWM Settings */
    pedal_phaser = util_pedal_pico_init(PEDAL_PHASER_PWM_1_GPIO, PEDAL_PHASER_PWM_2_GPIO);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_phaser_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0xF0);
    pwm_set_chan_level(pedal_phaser->pwm_1_slice, pedal_phaser->pwm_1_channel, PEDAL_PHASER_PWM_OFFSET);
    pwm_set_chan_level(pedal_phaser->pwm_2_slice, pedal_phaser->pwm_2_channel, PEDAL_PHASER_PWM_OFFSET);
    /* ADC Settings */
    util_pedal_pico_init_adc();
    /* Unique Settings */
    pedal_phaser_set();
    /* Start */
    util_pedal_pico_start((util_pedal_pico*)pedal_phaser);
    util_pedal_pico_sw_loop(PEDAL_PHASER_SW_1_GPIO, PEDAL_PHASER_SW_2_GPIO);
}

void pedal_phaser_set() {
    pedal_phaser_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_phaser_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_phaser_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_phaser_coefficient_swing = PEDAL_PHASER_COEFFICIENT_SWING_PEAK_FIXED_1;
    pedal_phaser_delay_x_1 = (int16*)calloc(PEDAL_PHASER_DELAY_TIME_MAX, sizeof(int16));
    pedal_phaser_delay_y_1 = (int16*)calloc(PEDAL_PHASER_DELAY_TIME_MAX, sizeof(int16));
    pedal_phaser_delay_x_2 = (int16*)calloc(PEDAL_PHASER_DELAY_TIME_MAX, sizeof(int16));
    pedal_phaser_delay_y_2 = (int16*)calloc(PEDAL_PHASER_DELAY_TIME_MAX, sizeof(int16));
    pedal_phaser_delay_time = PEDAL_PHASER_DELAY_TIME_FIXED_1;
    pedal_phaser_delay_index = 0;
    pedal_phaser_osc_sine_1_index = 0;
    pedal_phaser_osc_speed = pedal_phaser_conversion_2 >> 7; // Make 5-bit Value (0-31)
    pedal_phaser_osc_start_threshold = (pedal_phaser_conversion_3 >> 7) * PEDAL_PHASER_OSC_START_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    pedal_phaser_osc_start_count = 0;
}

void pedal_phaser_on_pwm_irq_wrap() {
    pwm_clear_irq(pedal_phaser->pwm_1_slice);
    //uint32 from_time = time_us_32();
    uint16 conversion_1 = util_pedal_pico_on_adc_conversion_1;
    uint16 conversion_2 = util_pedal_pico_on_adc_conversion_2;
    uint16 conversion_3 = util_pedal_pico_on_adc_conversion_3;
    if (! util_pedal_pico_on_adc_is_outstanding) {
        util_pedal_pico_on_adc_is_outstanding = true;
        adc_select_input(0); // Ensure to Start from ADC0
        __dsb();
        __isb();
        adc_run(true); // Stable Starting Point after PWM IRQ
    }
    util_pedal_pico_renew_adc_middle_moving_average(conversion_1);
    pedal_phaser_process(conversion_1, conversion_2, conversion_3);
    /* Output */
    pwm_set_chan_level(pedal_phaser->pwm_1_slice, pedal_phaser->pwm_1_channel, (uint16)pedal_phaser->output_1);
    pwm_set_chan_level(pedal_phaser->pwm_2_slice, pedal_phaser->pwm_2_channel, (uint16)pedal_phaser->output_1_inverted);
    //pedal_phaser_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_phaser_debug_time); // To send a made pointer, sync flag, etc.
    __dsb();
}

void pedal_phaser_process(uint16 conversion_1, uint16 conversion_2, uint16 conversion_3) {
    pedal_phaser_conversion_1 = conversion_1;
    if (abs(conversion_2 - pedal_phaser_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_phaser_conversion_2 = conversion_2;
        pedal_phaser_osc_speed = pedal_phaser_conversion_2 >> 7; // Make 5-bit Value (0-31)
    }
    if (abs(conversion_3 - pedal_phaser_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_phaser_conversion_3 = conversion_3;
        pedal_phaser_osc_start_threshold = (pedal_phaser_conversion_3 >> 7) * PEDAL_PHASER_OSC_START_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    }
    int32 normalized_1 = (int32)pedal_phaser_conversion_1 - (int32)util_pedal_pico_adc_middle_moving_average;
    /**
     * pedal_phaser_osc_start_count:
     *
     * Over Positive Threshold       ## 1
     *-----------------------------------------------------------------------------------------------------------
     * Under Positive Threshold     # 0 # 2      ### Reset to 1
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                  # 0   # 3   # 5   # 2
     *-----------------------------------------------------------------------------------------------------------
     * 0                           # 0   # 4   # 4   # 3   # 5 ...Count Up to PEDAL_PHASER_OSC_START_COUNT_MAX
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                         # 5 # 3      #### 4
     *-----------------------------------------------------------------------------------------------------------
     * Under Negative Threshold           # 6 # 2
     *-----------------------------------------------------------------------------------------------------------
     * Over Negative Threshold             ## Reset to 1
     */
    if (normalized_1 > pedal_phaser_osc_start_threshold || normalized_1 < -pedal_phaser_osc_start_threshold) {
        pedal_phaser_osc_start_count = 1;
    } else if (pedal_phaser_osc_start_count != 0 && (normalized_1 > (pedal_phaser_osc_start_threshold >> 1) || normalized_1 < -(pedal_phaser_osc_start_threshold >> 1))) {
        pedal_phaser_osc_start_count = 1;
    } else if (pedal_phaser_osc_start_count != 0) {
        pedal_phaser_osc_start_count++;
    }
    if (pedal_phaser_osc_start_count >= PEDAL_PHASER_OSC_START_COUNT_MAX) pedal_phaser_osc_start_count = 0;
    if (pedal_phaser_osc_start_count == 0) {
        pedal_phaser_osc_sine_1_index = 0;
    }
    /* Get Oscillator */
    int32 fixed_point_value_sine_1 = util_pedal_pico_ex_table_sine_1[pedal_phaser_osc_sine_1_index / PEDAL_PHASER_OSC_SINE_1_TIME_MULTIPLIER];
    pedal_phaser_osc_sine_1_index += pedal_phaser_osc_speed;
    if (pedal_phaser_osc_sine_1_index >= PEDAL_PHASER_OSC_SINE_1_TIME_MAX * PEDAL_PHASER_OSC_SINE_1_TIME_MULTIPLIER) pedal_phaser_osc_sine_1_index -= (PEDAL_PHASER_OSC_SINE_1_TIME_MAX * PEDAL_PHASER_OSC_SINE_1_TIME_MULTIPLIER);
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
     normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)util_pedal_pico_ex_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_PHASER_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    /**
     * Phaser is the synthesis of the concurrent wave and the phase shifted concurrent wave.
     * The phase shifted concurrent wave is made by an all-pass filter.
     *
     * Transfer Function (Z Transform) of All-Pass Filter:
     * H[Z] = frac{Y[Z]}{X[Z]} = frac{K + Z^-1}{1 + K*Z^-1}: Where Z is e^(j*omega*T) = complex number, K is coefficient (-1 <= K <= 1).
     * Y[Z] = X[Z]*Z^-1 - K*Y[Z]*Z^-1 + K*X[Z]
     * In the inverse Z transform to the differential equation for the discreate-time signal,
     * X[Z]*Z^-1 becomes X[S - 1], and Y[Z]*Z^-1 becomes Y[S - 1], where S is the current sample.
     * Y[S] = X[S - 1] - K*Y[S - 1] + K*X[S]: I use this.
     * Y[S] = X[S - 1] + K'*Y[S - 1] - K'*X[S]: Where K' = -K.
     *
     * All-pass filter is the combination of the low-pass filter and the high-pass filter on the same stage.
     * Low-pass Filter: Y[S] = K''*Y[S - 1] + (1 - K'')*X[S]: It's like a speaker in a closed box.
     * High-pass Filter: Y[S] = K''*X[S - 1] + (1 - K'')*X[S]: It's like a speaker with a reflection board.
     * All-pass Filter: Low-pass Filter + High-pass Filter - X[S]: K''*Y[S - 1] + K''*X[S - 1] + X[S] - 2*K''*X[S]
     * All-pass Filter with K'' = 1: X[S - 1] + Y[S - 1] - X[S]: With unknown K' = 1, this function actually makes a phase shift.
     * All-pass Filter Y(S) with S{5,-5,5,-5} where K' = 1 and preceding Y(S - 1) = 0 results Y{-5,5,-5,5}. This means phase shift 180 degrees delay.
     * X[S - 1] and Y[S - 1] are effected on a high frequency.
     * To get the effect on intended frequencies, use X[S - N] and Y[S - N] where N is the number of delay.
     * If the sampling frequency is 30518Hz, the effective frequency is its Nyquist (or folding) frequency and over, i.e., 15259Hz <=.
     * In this case, the most effective frequency is 15259Hz, and other frequencies far from the frequency isn't effected.
     * This phenomenon can also use for canceling noise at the intended frequency.
     */
    /* First Stage All-pass Filter for Noise Cancel */
    int16 delay_x_1 = pedal_phaser_delay_x_1[((pedal_phaser_delay_index + PEDAL_PHASER_DELAY_TIME_MAX) - pedal_phaser_delay_time) % PEDAL_PHASER_DELAY_TIME_MAX];
    int16 delay_y_1 = pedal_phaser_delay_y_1[((pedal_phaser_delay_index + PEDAL_PHASER_DELAY_TIME_MAX) - pedal_phaser_delay_time) % PEDAL_PHASER_DELAY_TIME_MAX];
    //if (pedal_phaser_delay_time) delay_x = 0; // No Delay, Otherwise Latest
    int32 phase_shift_1 = (int32)((int64)(((int64)delay_x_1 << 32) - ((int64)delay_y_1 << 32) + ((int64)normalized_1 << 32)) >> 32); // Coefficient = 1
    int32 canceled_1 = (normalized_1 + phase_shift_1) >> 1;
    pedal_phaser_delay_x_1[pedal_phaser_delay_index] = (int16)normalized_1;
    pedal_phaser_delay_y_1[pedal_phaser_delay_index] = (int16)phase_shift_1;
    /* Second Stage All-pass Filter for Phaser */
    int16 delay_x_2 = pedal_phaser_delay_x_2[((pedal_phaser_delay_index + PEDAL_PHASER_DELAY_TIME_MAX) - pedal_phaser_delay_time) % PEDAL_PHASER_DELAY_TIME_MAX];
    int16 delay_y_2 = pedal_phaser_delay_y_2[((pedal_phaser_delay_index + PEDAL_PHASER_DELAY_TIME_MAX) - pedal_phaser_delay_time) % PEDAL_PHASER_DELAY_TIME_MAX];
    int32 coefficient = (int32)(int64)(((int64)pedal_phaser_coefficient_swing * (int64)fixed_point_value_sine_1) >> 16); // Remain Decimal Part
    int32 phase_shift_2 = (int32)((int64)(((int64)delay_x_2 << 32) - (((int64)delay_y_2 << 16) * (int64)coefficient) + (((int64)canceled_1 << 16) * (int64)coefficient)) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    pedal_phaser_delay_x_2[pedal_phaser_delay_index] = (int16)canceled_1;
    pedal_phaser_delay_y_2[pedal_phaser_delay_index] = (int16)phase_shift_2;
    pedal_phaser_delay_index++;
    if (pedal_phaser_delay_index >= PEDAL_PHASER_DELAY_TIME_MAX) pedal_phaser_delay_index -= PEDAL_PHASER_DELAY_TIME_MAX;
    int32 mixed_1 = (canceled_1 - phase_shift_2) >> 1;
    if (util_pedal_pico_sw_mode == 1) {
        pedal_phaser_delay_time = PEDAL_PHASER_DELAY_TIME_FIXED_1;
    } else if (util_pedal_pico_sw_mode == 2) {
        pedal_phaser_delay_time = PEDAL_PHASER_DELAY_TIME_FIXED_3;
    } else {
        pedal_phaser_delay_time = PEDAL_PHASER_DELAY_TIME_FIXED_2;
    }
    mixed_1 *= PEDAL_PHASER_GAIN;
    pedal_phaser->output_1 = util_pedal_pico_cutoff_biased(mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_PHASER_PWM_OFFSET + PEDAL_PHASER_PWM_PEAK, PEDAL_PHASER_PWM_OFFSET - PEDAL_PHASER_PWM_PEAK);
    pedal_phaser->output_1_inverted = util_pedal_pico_cutoff_biased(-mixed_1 + (int32)util_pedal_pico_adc_middle_moving_average, PEDAL_PHASER_PWM_OFFSET + PEDAL_PHASER_PWM_PEAK, PEDAL_PHASER_PWM_OFFSET - PEDAL_PHASER_PWM_PEAK);
}

void pedal_phaser_free() { // Free Except Object, pedal_phaser
    util_pedal_pico_stop((util_pedal_pico*)pedal_phaser);
    irq_remove_handler(PWM_IRQ_WRAP, pedal_phaser_on_pwm_irq_wrap);
    free((void*)pedal_phaser_delay_x_1);
    free((void*)pedal_phaser_delay_y_1);
    free((void*)pedal_phaser_delay_x_2);
    free((void*)pedal_phaser_delay_y_2);
    __dsb();
}
