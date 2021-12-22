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
// Dependancies
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/sync.h"
// raspi_pico/include
#include "macros_pico.h"

#define SERIAL_CONSOLE_UART_ID uart1
#define SERIAL_CONSOLE_BAUD_RATE 1200
#define SERIAL_CONSOLE_UART_TX_GPIO 4
#define SERIAL_CONSOLE_UART_RX_GPIO 5
#define SERIAL_CONSOLE_PERIODIC_MICROSECONDS 12500
#define SERIAL_CONSOLE_SEQUENCER_PROGRAM_COUNTUPTO 64
#define SERIAL_CONSOLE_SEQUENCER_PROGRAM_LENGTH 3
#define SERIAL_CONSOLE_SEQUENCER_CORE_1_STACK_SIZE 1024

uint8_t serial_console_sequence[SERIAL_CONSOLE_SEQUENCER_PROGRAM_LENGTH][SERIAL_CONSOLE_SEQUENCER_PROGRAM_COUNTUPTO] = {
   {_64('X')},
   {_64('Y')},
   {_64('P')}
};

volatile uint16_t serial_console_sequence_index;
volatile uint16_t serial_console_sequence_count;
volatile bool serial_console_sequence_is_start;


void serial_console_core_1();
bool serial_console_on_repeating_timer_callback(struct repeating_timer *t);

int main(void) {
    stdio_usb_init(); // No Use UART for STDIO
    sleep_ms(2000); // Wait for Rediness of USB for Messages
    /* Initialize Global Variables */
    serial_console_sequence_index = 0;
    serial_console_sequence_count = 0;
    serial_console_sequence_is_start = false;
    /* UART Settings */
    uint32_t baud_rate = uart_init(SERIAL_CONSOLE_UART_ID, SERIAL_CONSOLE_BAUD_RATE);
    printf("@main 1 - baud_rate: %d\n", baud_rate);
    gpio_set_function(SERIAL_CONSOLE_UART_TX_GPIO, GPIO_FUNC_UART);
    gpio_set_function(SERIAL_CONSOLE_UART_RX_GPIO, GPIO_FUNC_UART);
    /* Launch Core 1 */
    uint32_t* stack_pointer = (int32_t*)malloc(SERIAL_CONSOLE_SEQUENCER_CORE_1_STACK_SIZE);
    multicore_launch_core1_with_stack(serial_console_core_1, stack_pointer, SERIAL_CONSOLE_SEQUENCER_CORE_1_STACK_SIZE);
    while (true) {
        puts("Type Sequence Index 0-2 or Type E to Stop Sequence:");
        int32_t input = getchar_timeout_us(10000000);
        if (input == PICO_ERROR_TIMEOUT) continue;
        uint8_t input_char = (uint8_t)input;
        if (input_char >= 0x30 && input_char <= 0x39) { // "0-9" to 0-9
            printf("%c\n", input_char);
            input_char -= 0x30;
            if (! serial_console_sequence_is_start) {
                serial_console_sequence_index = _min(input_char, SERIAL_CONSOLE_SEQUENCER_PROGRAM_LENGTH - 1);
                serial_console_sequence_count = 0;
                serial_console_sequence_is_start = true;
            } else {
                serial_console_sequence_index = _min(input_char, SERIAL_CONSOLE_SEQUENCER_PROGRAM_LENGTH - 1);
            }
        } else if (input_char == 'E') {
            if (serial_console_sequence_is_start) serial_console_sequence_is_start = false;
        } else {
            continue;
        }
    }
    return 0;
}

void serial_console_core_1() {
    /* Timer Settings */
    struct repeating_timer timer;
    add_repeating_timer_us(-SERIAL_CONSOLE_PERIODIC_MICROSECONDS, serial_console_on_repeating_timer_callback, NULL, &timer); // Add "-" to First Argument for Periodical Callbacks without Considering Execution Time of Callback
    while (true) {
        tight_loop_contents();
    }
}

bool serial_console_on_repeating_timer_callback(struct repeating_timer *t) {
    //if (uart_is_readable(SERIAL_CONSOLE_UART_ID)) printf("@serial_console_on_repeating_timer_callback 1 - uart_getc: %c\n", uart_getc(SERIAL_CONSOLE_UART_ID));
    if (serial_console_sequence_is_start) {
        uint8_t output_char = serial_console_sequence[serial_console_sequence_index][serial_console_sequence_count];
        if (uart_is_writable(SERIAL_CONSOLE_UART_ID)) {
            uart_putc_raw(SERIAL_CONSOLE_UART_ID, output_char);
        } else {
            puts("No Writable UART Tx!");
        }
        //hw_set_bits(&uart1_hw->dr, output_char);
        //printf("@serial_console_on_repeating_timer_callback 2 - output_char: %c\n", output_char);
        if (++serial_console_sequence_count >= SERIAL_CONSOLE_SEQUENCER_PROGRAM_COUNTUPTO) serial_console_sequence_count = 0;
        //printf("@serial_console_on_repeating_timer_callback 3 - serial_console_sequence_count: %d\n", serial_console_sequence_count);
    }
    __dsb();
}

