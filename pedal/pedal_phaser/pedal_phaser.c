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
// raspi_pico/include
#include "macros_pico.h"
// Private header
#include "pedal_phaser.h"

#define PEDAL_PHASER_LED_GPIO 25
#define PEDAL_PHASER_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_PHASER_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_PHASER_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_PHASER_PWM_PEAK 2047
#define PEDAL_PHASER_GAIN 2
#define PEDAL_PHASER_OSC_SINE_1_TIME_MAX 30518
#define PEDAL_PHASER_COEFFICIENT_SWING_PEAK_1 (int32)(0x00010000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_PHASER_COEFFICIENT_SWING_SHIFT 12 // Multiply By 4096 (0x1 - 0x10 to 0x1 - 0x00010000)
#define PEDAL_PHASER_DELAY_TIME_MAX 64
#define PEDAL_PHASER_DELAY_TIME_FIXED_1 32 // 32 Divided by 30518 (0.001 Seconds on 90 Degrees = 250Hz)
#define PEDAL_PHASER_ADC_0_GPIO 26
#define PEDAL_PHASER_ADC_1_GPIO 27
#define PEDAL_PHASER_ADC_2_GPIO 28
#define PEDAL_PHASER_ADC_MIDDLE_DEFAULT 2048
#define PEDAL_PHASER_ADC_MIDDLE_NUMBER_MOVING_AVERAGE 16384 // Should be Power of 2 Because of Processing Speed (Logical Shift Left on Division)
#define PEDAL_PHASER_ADC_THRESHOLD 0x7F // Range is 0x0-0xFFF (0-4095) Divided by 0xFF (255) for 0x0-0xFb (0-15). 0xFF >> 1.

uint32 pedal_phaser_pwm_slice_num;
uint32 pedal_phaser_pwm_channel;
uint16 pedal_phaser_conversion_1;
uint16 pedal_phaser_conversion_2;
uint16 pedal_phaser_conversion_3;
uint16 pedal_phaser_conversion_1_temp;
uint16 pedal_phaser_conversion_2_temp;
uint16 pedal_phaser_conversion_3_temp;
uint16 pedal_phaser_osc_sine_1_index;
uint16 pedal_phaser_osc_speed;
int32 pedal_phaser_coefficient_swing;
int16* pedal_phaser_delay_x;
int16* pedal_phaser_delay_y;
uint16 pedal_phaser_delay_time;
uint16 pedal_phaser_delay_index;
uint32 pedal_phaser_adc_middle_moving_average;
bool pedal_phaser_is_outstanding_on_adc;
uint32 pedal_phaser_debug_time;

void pedal_phaser_core_1();
void pedal_phaser_on_pwm_irq_wrap();
void pedal_phaser_on_adc_irq_fifo();

int main(void) {
    //stdio_init_all();
    //sleep_ms(2000); // Wait for Rediness of USB for Messages
    gpio_init(PEDAL_PHASER_LED_GPIO);
    gpio_set_dir(PEDAL_PHASER_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_PHASER_LED_GPIO, true);
    multicore_launch_core1(pedal_phaser_core_1);
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
        tight_loop_contents();
    }
    return 0;
}

void pedal_phaser_core_1() {
    /* PWM Settings */
    gpio_set_function(PEDAL_PHASER_PWM_1_GPIO, GPIO_FUNC_PWM); // GPIO16 = PWM8 A
    gpio_set_function(PEDAL_PHASER_PWM_2_GPIO, GPIO_FUNC_PWM); // GPIO17 = PWM8 B
    pedal_phaser_pwm_slice_num = pwm_gpio_to_slice_num(PEDAL_PHASER_PWM_1_GPIO); // GPIO16 = PWM8
    pedal_phaser_pwm_channel = pwm_gpio_to_channel(PEDAL_PHASER_PWM_1_GPIO); // GPIO16 = A
    // Set IRQ and Handler for PWM
    pwm_clear_irq(pedal_phaser_pwm_slice_num);
    pwm_set_irq_enabled(pedal_phaser_pwm_slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_phaser_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0xF0); // Higher Priority
    // PWM Configuration (Make Approx. 30518Hz from 125Mhz - 0.032768ms Cycle)
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    pwm_config_set_clkdiv(&config, 1.0f); // Set Clock Divider, 125,000,000 Divided by 1.0 for 0.008us Cycle
    pwm_config_set_wrap(&config, 4095); // 0-4095, 4096 Cycles for 0.032768ms
    pwm_init(pedal_phaser_pwm_slice_num, &config, false); // Push Configufatio
    pwm_set_chan_level(pedal_phaser_pwm_slice_num, pedal_phaser_pwm_channel, PEDAL_PHASER_PWM_OFFSET); // Set Channel A
    pwm_set_chan_level(pedal_phaser_pwm_slice_num, pedal_phaser_pwm_channel + 1, PEDAL_PHASER_PWM_OFFSET); // Set Channel B
    /* ADC Settings */
    adc_init();
    adc_gpio_init(PEDAL_PHASER_ADC_0_GPIO); // GPIO26 (ADC0)
    adc_gpio_init(PEDAL_PHASER_ADC_1_GPIO); // GPIO27 (ADC1)
    adc_gpio_init(PEDAL_PHASER_ADC_2_GPIO); // GPIO28 (ADC2)
    adc_set_clkdiv(0.0f);
    adc_set_round_robin(0b00111);
    adc_fifo_setup(true, false, 3, true, false); // 12-bit Length (0-4095), Bit[15] for Error Flag
    adc_fifo_drain(); // Clean FIFO
    irq_set_exclusive_handler(ADC_IRQ_FIFO, pedal_phaser_on_adc_irq_fifo);
    irq_set_priority(ADC_IRQ_FIFO, 0xFF); // Highest Priority
    adc_irq_set_enabled(true);
    pedal_phaser_conversion_1 = PEDAL_PHASER_ADC_MIDDLE_DEFAULT;
    pedal_phaser_conversion_2 = PEDAL_PHASER_ADC_MIDDLE_DEFAULT;
    pedal_phaser_conversion_3 = PEDAL_PHASER_ADC_MIDDLE_DEFAULT;
    pedal_phaser_conversion_1_temp = PEDAL_PHASER_ADC_MIDDLE_DEFAULT;
    pedal_phaser_conversion_2_temp = PEDAL_PHASER_ADC_MIDDLE_DEFAULT;
    pedal_phaser_conversion_3_temp = PEDAL_PHASER_ADC_MIDDLE_DEFAULT;
    pedal_phaser_adc_middle_moving_average = pedal_phaser_conversion_1 * PEDAL_PHASER_ADC_MIDDLE_NUMBER_MOVING_AVERAGE;
    pedal_phaser_osc_speed = pedal_phaser_conversion_2 >> 8; // Make 4-bit Value (0-15)
    pedal_phaser_osc_sine_1_index = 0;
    pedal_phaser_coefficient_swing = ((pedal_phaser_conversion_3 >> 8) + 1) << PEDAL_PHASER_COEFFICIENT_SWING_SHIFT; // Make 4-bit Value (0-15) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    pedal_phaser_delay_x = (int16*)calloc(PEDAL_PHASER_DELAY_TIME_MAX, sizeof(int16));
    pedal_phaser_delay_y = (int16*)calloc(PEDAL_PHASER_DELAY_TIME_MAX, sizeof(int16));
    pedal_phaser_delay_time = PEDAL_PHASER_DELAY_TIME_FIXED_1;
    pedal_phaser_delay_index = 0;
    /* Start IRQ, PWM and ADC */
    irq_set_mask_enabled(0b1 << PWM_IRQ_WRAP|0b1 << ADC_IRQ_FIFO, true);
    pwm_set_mask_enabled(0b1 << pedal_phaser_pwm_slice_num);
    pedal_phaser_is_outstanding_on_adc = true;
    adc_select_input(0); // Ensure to Start from A0
    __dsb();
    __isb();
    adc_run(true);
    while (true) {
        tight_loop_contents();
    }
}

void pedal_phaser_on_pwm_irq_wrap() {
    //uint32 from_time = time_us_32();
    uint16 conversion_1_temp = pedal_phaser_conversion_1_temp;
    uint16 conversion_2_temp = pedal_phaser_conversion_2_temp;
    uint16 conversion_3_temp = pedal_phaser_conversion_3_temp;
    if (! pedal_phaser_is_outstanding_on_adc) {
        pedal_phaser_is_outstanding_on_adc = true;
        adc_select_input(0); // Ensure to Start from A0
        __dsb();
        __isb();
        adc_run(true); // Stable Starting Point after PWM IRQ
    }
    pedal_phaser_conversion_1 = conversion_1_temp;
    if (abs(conversion_2_temp - pedal_phaser_conversion_2) > PEDAL_PHASER_ADC_THRESHOLD) {
        pedal_phaser_conversion_2 = conversion_2_temp;
        pedal_phaser_osc_speed = pedal_phaser_conversion_2 >> 8; // Make 4-bit Value (0-15)
    }
    if (abs(conversion_3_temp - pedal_phaser_conversion_3) > PEDAL_PHASER_ADC_THRESHOLD) {
        pedal_phaser_conversion_3 = conversion_3_temp;
        pedal_phaser_coefficient_swing = ((pedal_phaser_conversion_3 >> 8) + 1) << PEDAL_PHASER_COEFFICIENT_SWING_SHIFT; // Make 4-bit Value (0-15) and Shift for 32-bit Signed (Two's Compliment) Fixed Decimal
    }
    uint32 middle_moving_average = pedal_phaser_adc_middle_moving_average / PEDAL_PHASER_ADC_MIDDLE_NUMBER_MOVING_AVERAGE;
    pedal_phaser_adc_middle_moving_average -= middle_moving_average;
    pedal_phaser_adc_middle_moving_average += pedal_phaser_conversion_1;
    int32 normalized_1 = (int32)pedal_phaser_conversion_1 - (int32)middle_moving_average;
    /* Get Oscillator */
    int32 fixed_point_value_sine_1 = pedal_phaser_table_sine_1[pedal_phaser_osc_sine_1_index];
    pedal_phaser_osc_sine_1_index += pedal_phaser_osc_speed;
    if (pedal_phaser_osc_sine_1_index >= PEDAL_PHASER_OSC_SINE_1_TIME_MAX) pedal_phaser_osc_sine_1_index -= PEDAL_PHASER_OSC_SINE_1_TIME_MAX;
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    /**
     * Phaser is the addition of the concurrent wave and the phase shifted concurrent wave.
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
     * If K'' = 1, this means the phase shift 90 degress.
     * All-pass Filter with K'' = 1: X[S - 1] + Y[S - 1] - X[S]: With unknown K' = 1, this function actually makes a phase shift.
     * X[S - 1] and Y[S - 1] are effected on a high frequency.
     * To get effect on intended frequencies, use X[S - N] and Y[S - N] where N is the number of delay.
     */
    int32 coefficient = (int32)(int64)(((int64)(pedal_phaser_coefficient_swing) * (int64)fixed_point_value_sine_1) >> 16); // Remain Decimal Part
    int16 delay_x = pedal_phaser_delay_x[((pedal_phaser_delay_index + PEDAL_PHASER_DELAY_TIME_MAX) - pedal_phaser_delay_time) % PEDAL_PHASER_DELAY_TIME_MAX];
    int16 delay_y = pedal_phaser_delay_y[((pedal_phaser_delay_index + PEDAL_PHASER_DELAY_TIME_MAX) - pedal_phaser_delay_time) % PEDAL_PHASER_DELAY_TIME_MAX];
    //if (pedal_phaser_delay_time) delay_x = 0; // No Delay, Otherwise Latest
    int32 phase_shift_1 = (int32)delay_x - (int32)(int64)(((int64)(delay_y << 16) * (int64)coefficient) >> 32) + (int32)(int64)(((int64)(normalized_1 << 16) * (int64)coefficient) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    pedal_phaser_delay_x[pedal_phaser_delay_index] = (int16)normalized_1;
    pedal_phaser_delay_y[pedal_phaser_delay_index] = (int16)phase_shift_1;
    pedal_phaser_delay_index++;
    if (pedal_phaser_delay_index >= PEDAL_PHASER_DELAY_TIME_MAX) pedal_phaser_delay_index = 0;
    int32 mixed_1 = normalized_1 + phase_shift_1;
    mixed_1 *= PEDAL_PHASER_GAIN;
    int32 output_1 = mixed_1 + middle_moving_average;
    if (output_1 > PEDAL_PHASER_PWM_OFFSET + PEDAL_PHASER_PWM_PEAK) {
        output_1 = PEDAL_PHASER_PWM_OFFSET + PEDAL_PHASER_PWM_PEAK;
    } else if (output_1 < PEDAL_PHASER_PWM_OFFSET - PEDAL_PHASER_PWM_PEAK) {
        output_1 = PEDAL_PHASER_PWM_OFFSET - PEDAL_PHASER_PWM_PEAK;
    }
    int32 output_1_inverted = -mixed_1 + middle_moving_average;
    if (output_1_inverted > PEDAL_PHASER_PWM_OFFSET + PEDAL_PHASER_PWM_PEAK) {
        output_1_inverted = PEDAL_PHASER_PWM_OFFSET + PEDAL_PHASER_PWM_PEAK;
    } else if (output_1_inverted < PEDAL_PHASER_PWM_OFFSET - PEDAL_PHASER_PWM_PEAK) {
        output_1_inverted = PEDAL_PHASER_PWM_OFFSET - PEDAL_PHASER_PWM_PEAK;
    }
    pwm_set_chan_level(pedal_phaser_pwm_slice_num, pedal_phaser_pwm_channel, (uint16)output_1);
    pwm_set_chan_level(pedal_phaser_pwm_slice_num, pedal_phaser_pwm_channel + 1, (uint16)output_1_inverted);
    pwm_clear_irq(pedal_phaser_pwm_slice_num); // Seems Overlap IRQ Otherwise
    //pedal_phaser_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_phaser_debug_time); // To send a made pointer, sync flag, etc.
}

void pedal_phaser_on_adc_irq_fifo() {
    adc_run(false);
    uint16 adc_fifo_level = adc_fifo_get_level(); // Seems 8 at Maximum
    //printf("@pedal_phaser_on_adc_irq_fifo 1 - adc_fifo_level: %d\n", adc_fifo_level);
    for (uint16 i = 0; i < adc_fifo_level; i++) {
        //printf("@pedal_phaser_on_adc_irq_fifo 2 - i: %d\n", i);
        uint16 temp = adc_fifo_get();
        temp &= 0x7FFF; // Clear Bit[15]: ERR
        uint16 remainder = i % 3;
        if (remainder == 2) {
            pedal_phaser_conversion_3_temp = temp;
        } else if (remainder == 1) {
            pedal_phaser_conversion_2_temp = temp;
        } else if (remainder == 0) {
            pedal_phaser_conversion_1_temp = temp;
        }
    }
    //printf("@pedal_phaser_on_adc_irq_fifo 3 - adc_fifo_is_empty(): %d\n", adc_fifo_is_empty());
    adc_fifo_drain();
    pedal_phaser_is_outstanding_on_adc = false;
}
