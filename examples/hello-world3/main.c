/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "board.h"
#include "periph_cpu.h"
#include "periph_conf.h"
#include "periph/uart.h"

static void rx_cb(void *arg, uint8_t data) {
    (void) arg;
    if(data == '1') {
        LED0_ON;
    }
    else {
        LED0_OFF;
    }
}

int main(void)
{
    uart_t uart = 0;
    uint32_t baudrate = 115200;
    uint8_t state = 1u;
    (void) state;

    uart_init(uart, baudrate, &rx_cb, NULL);

    puts("Hello World!");
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

#ifdef LED0_TOGGLE
    while (1) {
        for (volatile uint32_t i = 0; i < 1250000; i++) { }
        LED0_TOGGLE;
        //uart_write(uart, &state, sizeof(state));
        //state = (state == 1u) ? 0u : 1u;
    }
#endif

    uart_poweroff(uart);

    return 0;
}
