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
#include "util_pedal_pico.h"
// Private header
#include "pedal_buffer.h"

#define PEDAL_BUFFER_TRANSIENT_RESPONSE 100000 // 100000 Micro Seconds
#define PEDAL_BUFFER_CORE_1_STACK_SIZE 1024 * 4 // 1024 Words, 4096 Bytes
#define PEDAL_BUFFER_LED_GPIO 25
#define PEDAL_BUFFER_SW_1_GPIO 14
#define PEDAL_BUFFER_SW_2_GPIO 15
#define PEDAL_BUFFER_PWM_1_GPIO 16 // Should Be Channel A of PWM (Same as Second)
#define PEDAL_BUFFER_PWM_2_GPIO 17 // Should Be Channel B of PWM (Same as First)
#define PEDAL_BUFFER_PWM_OFFSET 2048 // Ideal Middle Point
#define PEDAL_BUFFER_PWM_PEAK 2047
#define PEDAL_BUFFER_GAIN 1
#define PEDAL_BUFFER_DELAY_TIME_MAX 7937
#define PEDAL_BUFFER_DELAY_TIME_SHIFT 8 // Multiply By 256 (0-7936), 7936 Divided by 28125 (0.282 Seconds)
#define PEDAL_BUFFER_DELAY_TIME_INTERPOLATION_ACCUM 1
#define PEDAL_BUFFER_DELAY_AMPLITUDE_FIXED_1 (int32)(0x00004000) // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
#define PEDAL_BUFFER_DELAY_AMPLITUDE_FIXED_2 (int32)(0x00008000)
#define PEDAL_BUFFER_DELAY_AMPLITUDE_FIXED_3 (int32)(0x0000F000)
#define PEDAL_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_1 0x2
#define PEDAL_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_2 0x4
#define PEDAL_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_3 0x8
#define PEDAL_BUFFER_NOISE_GATE_REDUCE_SHIFT 6 // Divide by 64
#define PEDAL_BUFFER_NOISE_GATE_THRESHOLD_MULTIPLIER 1 // From -66.22dB (Loss 2047) to -36.39dB (Loss 66) in ADC_VREF (Typically 3.3V)
#define PEDAL_BUFFER_NOISE_GATE_COUNT_MAX 2000 // 28125 Divided by 2000 = Approx. 14Hz

volatile uint32 pedal_buffer_pwm_slice_num;
volatile uint32 pedal_buffer_pwm_channel;
volatile uint16 pedal_buffer_conversion_1;
volatile uint16 pedal_buffer_conversion_2;
volatile uint16 pedal_buffer_conversion_3;
volatile int16* pedal_buffer_delay_array;
volatile int32 pedal_buffer_delay_amplitude; // Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part
volatile int32 pedal_buffer_delay_amplitude_interpolation;
volatile uchar8 pedal_buffer_delay_amplitude_interpolation_accum;
volatile uint16 pedal_buffer_delay_time;
volatile uint16 pedal_buffer_delay_time_interpolation;
volatile uint16 pedal_buffer_delay_index;
volatile char8 pedal_buffer_noise_gate_threshold;
volatile uint16 pedal_buffer_noise_gate_count;
volatile uint32 pedal_buffer_debug_time;

void pedal_buffer_core_1();
void pedal_buffer_on_pwm_irq_wrap();

int main(void) {
    //stdio_init_all();
    util_pedal_pico_set_sys_clock_115200khz();
    //stdio_init_all(); // Re-init for UART Baud Rate
    sleep_us(PEDAL_BUFFER_TRANSIENT_RESPONSE); // Pass through Transient Response of Power
    gpio_init(PEDAL_BUFFER_LED_GPIO);
    gpio_set_dir(PEDAL_BUFFER_LED_GPIO, GPIO_OUT);
    gpio_put(PEDAL_BUFFER_LED_GPIO, true);
    uint32* stack_pointer = (int32*)malloc(PEDAL_BUFFER_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(pedal_buffer_core_1, stack_pointer, PEDAL_BUFFER_CORE_1_STACK_SIZE);
    //pedal_buffer_debug_time = 0;
    //uint32 from_time = time_us_32();
    //printf("@main 1 - Let's Start!\n");
    //pedal_buffer_debug_time = time_us_32() - from_time;
    //printf("@main 2 - pedal_buffer_debug_time %d\n", pedal_buffer_debug_time);
    while (true) {
        //printf("@main 3 - pedal_buffer_conversion_1 %0x\n", pedal_buffer_conversion_1);
        //printf("@main 4 - pedal_buffer_conversion_2 %0x\n", pedal_buffer_conversion_2);
        //printf("@main 5 - pedal_buffer_conversion_3 %0x\n", pedal_buffer_conversion_3);
        //printf("@main 6 - multicore_fifo_pop_blocking() %d\n", multicore_fifo_pop_blocking());
        //printf("@main 7 - pedal_buffer_debug_time %d\n", pedal_buffer_debug_time);
        //sleep_ms(500);
        //tight_loop_contents();
        __wfi();
    }
    return 0;
}

void pedal_buffer_core_1() {
    /* PWM Settings */
    gpio_set_function(PEDAL_BUFFER_PWM_1_GPIO, GPIO_FUNC_PWM); // GPIO16 = PWM8 A
    gpio_set_function(PEDAL_BUFFER_PWM_2_GPIO, GPIO_FUNC_PWM); // GPIO17 = PWM8 B
    pedal_buffer_pwm_slice_num = pwm_gpio_to_slice_num(PEDAL_BUFFER_PWM_1_GPIO); // GPIO16 = PWM8
    pedal_buffer_pwm_channel = pwm_gpio_to_channel(PEDAL_BUFFER_PWM_1_GPIO); // GPIO16 = A
    // Set IRQ and Handler for PWM
    pwm_clear_irq(pedal_buffer_pwm_slice_num);
    pwm_set_irq_enabled(pedal_buffer_pwm_slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pedal_buffer_on_pwm_irq_wrap);
    irq_set_priority(PWM_IRQ_WRAP, 0xF0); // Higher Priority
    // PWM Configuration
    pwm_config config = pwm_get_default_config(); // Pull Configuration
    util_pedal_pico_set_pwm_28125hz(&config);
    pwm_init(pedal_buffer_pwm_slice_num, &config, false); // Push Configufatio
    pwm_set_chan_level(pedal_buffer_pwm_slice_num, pedal_buffer_pwm_channel, PEDAL_BUFFER_PWM_OFFSET); // Set Channel A
    pwm_set_chan_level(pedal_buffer_pwm_slice_num, pedal_buffer_pwm_channel + 1, PEDAL_BUFFER_PWM_OFFSET); // Set Channel B
    /* ADC Settings */
    util_pedal_pico_init_adc();
    pedal_buffer_conversion_1 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_buffer_conversion_2 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_buffer_conversion_3 = UTIL_PEDAL_PICO_ADC_MIDDLE_DEFAULT;
    pedal_buffer_delay_array = (int16*)calloc(PEDAL_BUFFER_DELAY_TIME_MAX, sizeof(int16));
    pedal_buffer_delay_amplitude = PEDAL_BUFFER_DELAY_AMPLITUDE_FIXED_1;
    pedal_buffer_delay_amplitude_interpolation = PEDAL_BUFFER_DELAY_AMPLITUDE_FIXED_1;
    pedal_buffer_delay_amplitude_interpolation_accum = PEDAL_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_1;
    uint16 delay_time = (pedal_buffer_conversion_2 >> 7) << PEDAL_BUFFER_DELAY_TIME_SHIFT; // Make 5-bit Value (0-31) and Shift
    pedal_buffer_delay_time = delay_time;
    pedal_buffer_delay_time_interpolation = delay_time;
    pedal_buffer_delay_index = 0;
    pedal_buffer_noise_gate_threshold = (pedal_buffer_conversion_3 >> 7) * PEDAL_BUFFER_NOISE_GATE_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    pedal_buffer_noise_gate_count = 0;
    /* Start IRQ, PWM and ADC */
    util_pedal_pico_sw_mode = 0; // Initialize Mode of Switch Before Running PWM and ADC
    irq_set_mask_enabled(0b1 << PWM_IRQ_WRAP|0b1 << ADC_IRQ_FIFO, true);
    pwm_set_mask_enabled(0b1 << pedal_buffer_pwm_slice_num);
    adc_select_input(0); // Ensure to Start from A0
    __dsb();
    __isb();
    adc_run(true);
    util_pedal_pico_sw_loop(PEDAL_BUFFER_SW_1_GPIO, PEDAL_BUFFER_SW_2_GPIO);
}

void pedal_buffer_on_pwm_irq_wrap() {
    pwm_clear_irq(pedal_buffer_pwm_slice_num);
    //uint32 from_time = time_us_32();
    uint16 conversion_1_temp = util_pedal_pico_on_adc_conversion_1;
    uint16 conversion_2_temp = util_pedal_pico_on_adc_conversion_2;
    uint16 conversion_3_temp = util_pedal_pico_on_adc_conversion_3;
    if (! util_pedal_pico_on_adc_is_outstanding) {
        util_pedal_pico_on_adc_is_outstanding = true;
        adc_select_input(0); // Ensure to Start from A0
        __dsb();
        __isb();
        adc_run(true); // Stable Starting Point after PWM IRQ
    }
    pedal_buffer_conversion_1 = conversion_1_temp;
    if (abs(conversion_2_temp - pedal_buffer_conversion_2) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_buffer_conversion_2 = conversion_2_temp;
        pedal_buffer_delay_time = (pedal_buffer_conversion_2 >> 7) << PEDAL_BUFFER_DELAY_TIME_SHIFT; // Make 5-bit Value (0-31) and Shift
    }
    if (abs(conversion_3_temp - pedal_buffer_conversion_3) > UTIL_PEDAL_PICO_ADC_THRESHOLD) {
        pedal_buffer_conversion_3 = conversion_3_temp;
        pedal_buffer_noise_gate_threshold = (pedal_buffer_conversion_3 >> 7) * PEDAL_BUFFER_NOISE_GATE_THRESHOLD_MULTIPLIER; // Make 5-bit Value (0-31) and Multiply
    }
    pedal_buffer_delay_time_interpolation = util_pedal_pico_interpolate(pedal_buffer_delay_time_interpolation, pedal_buffer_delay_time, PEDAL_BUFFER_DELAY_TIME_INTERPOLATION_ACCUM);
    uint32 middle_moving_average = util_pedal_pico_adc_middle_moving_average / UTIL_PEDAL_PICO_ADC_MIDDLE_MOVING_AVERAGE_NUMBER;
    util_pedal_pico_adc_middle_moving_average -= middle_moving_average;
    util_pedal_pico_adc_middle_moving_average += pedal_buffer_conversion_1;
    int32 normalized_1 = (int32)pedal_buffer_conversion_1 - (int32)middle_moving_average;
    /**
     * pedal_buffer_noise_gate_count:
     *
     * Over Positive Threshold       ## 1
     *-----------------------------------------------------------------------------------------------------------
     * Under Positive Threshold     # 0 # 2      ### Reset to 1
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                  # 0   # 3   # 5   # 2
     *-----------------------------------------------------------------------------------------------------------
     * 0                           # 0   # 4   # 4   # 3   # 5 ...Count Up to PEDAL_BUFFER_NOISE_GATE_COUNT_MAX
     *-----------------------------------------------------------------------------------------------------------
     * Hysteresis                         # 5 # 3      #### 4
     *-----------------------------------------------------------------------------------------------------------
     * Under Negative Threshold           # 6 # 2
     *-----------------------------------------------------------------------------------------------------------
     * Over Negative Threshold             ## Reset to 1
     */
    if (normalized_1 > pedal_buffer_noise_gate_threshold || normalized_1 < -pedal_buffer_noise_gate_threshold) {
        pedal_buffer_noise_gate_count = 1;
        pedal_buffer_delay_amplitude = 0x00000000;
    } else if (pedal_buffer_noise_gate_count != 0 && (normalized_1 > (pedal_buffer_noise_gate_threshold >> 1) || normalized_1 < -(pedal_buffer_noise_gate_threshold >> 1))) {
        pedal_buffer_noise_gate_count = 1;
    } else if (pedal_buffer_noise_gate_count != 0) {
        pedal_buffer_noise_gate_count++;
    }
    if (pedal_buffer_noise_gate_count >= PEDAL_BUFFER_NOISE_GATE_COUNT_MAX) {
        pedal_buffer_noise_gate_count = 0;
        if (util_pedal_pico_sw_mode == 1) {
            pedal_buffer_delay_amplitude = PEDAL_BUFFER_DELAY_AMPLITUDE_FIXED_1;
            pedal_buffer_delay_amplitude_interpolation_accum = PEDAL_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_1;
        } else if (util_pedal_pico_sw_mode == 2) {
            pedal_buffer_delay_amplitude = PEDAL_BUFFER_DELAY_AMPLITUDE_FIXED_3;
            pedal_buffer_delay_amplitude_interpolation_accum = PEDAL_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_3;
        } else {
            pedal_buffer_delay_amplitude = PEDAL_BUFFER_DELAY_AMPLITUDE_FIXED_2;
            pedal_buffer_delay_amplitude_interpolation_accum = PEDAL_BUFFER_DELAY_AMPLITUDE_INTERPOLATION_ACCUM_FIXED_2;
        }
    }
    pedal_buffer_delay_amplitude_interpolation = util_pedal_pico_interpolate(pedal_buffer_delay_amplitude_interpolation, pedal_buffer_delay_amplitude, pedal_buffer_delay_amplitude_interpolation_accum);
    if (pedal_buffer_noise_gate_count == 0) {
        normalized_1 >>= PEDAL_BUFFER_NOISE_GATE_REDUCE_SHIFT;
    }
    /**
     * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
     * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
     * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
     */
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_buffer_table_pdf_1[abs(util_pedal_pico_cutoff_normalized(normalized_1, PEDAL_BUFFER_PWM_PEAK))]) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication to Get Only Integer Part
    /* Make Sustain */
    int32 delay_1 = (int32)pedal_buffer_delay_array[((pedal_buffer_delay_index + PEDAL_BUFFER_DELAY_TIME_MAX) - pedal_buffer_delay_time_interpolation) % PEDAL_BUFFER_DELAY_TIME_MAX];
    if (pedal_buffer_delay_time_interpolation == 0) delay_1 = 0; // No Reverb, Otherwise Latest
    int32 pedal_buffer_normalized_1_amplitude = 0x00010000 - pedal_buffer_delay_amplitude_interpolation;
    normalized_1 = (int32)(int64)((((int64)normalized_1 << 16) * (int64)pedal_buffer_normalized_1_amplitude) >> 32);
    delay_1 = (int32)(int64)((((int64)delay_1 << 16) * (int64)pedal_buffer_delay_amplitude_interpolation) >> 32);
    int32 mixed_1 = normalized_1 + delay_1;
    pedal_buffer_delay_array[pedal_buffer_delay_index] = (int16)mixed_1;
    pedal_buffer_delay_index++;
    if (pedal_buffer_delay_index >= PEDAL_BUFFER_DELAY_TIME_MAX) pedal_buffer_delay_index -= PEDAL_BUFFER_DELAY_TIME_MAX;
    mixed_1 *= PEDAL_BUFFER_GAIN;
    /* Output */
    int32 output_1 = util_pedal_pico_cutoff_biased(mixed_1 + middle_moving_average, PEDAL_BUFFER_PWM_OFFSET + PEDAL_BUFFER_PWM_PEAK, PEDAL_BUFFER_PWM_OFFSET - PEDAL_BUFFER_PWM_PEAK);
    int32 output_1_inverted = util_pedal_pico_cutoff_biased(-mixed_1 + middle_moving_average, PEDAL_BUFFER_PWM_OFFSET + PEDAL_BUFFER_PWM_PEAK, PEDAL_BUFFER_PWM_OFFSET - PEDAL_BUFFER_PWM_PEAK);
    pwm_set_chan_level(pedal_buffer_pwm_slice_num, pedal_buffer_pwm_channel, (uint16)output_1);
    pwm_set_chan_level(pedal_buffer_pwm_slice_num, pedal_buffer_pwm_channel + 1, (uint16)output_1_inverted);
    //pedal_buffer_debug_time = time_us_32() - from_time;
    //multicore_fifo_push_blocking(pedal_buffer_debug_time); // To send a made pointer, sync flag, etc.
    __dsb();
}
