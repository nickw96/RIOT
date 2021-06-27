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
#include "periph_conf.h"

int main(void)
{
    puts("Hello World!");

    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

#if defined(LED0_TOGGLE) && (CLOCK_CORECLOCK)
    while (1) {
        /* Delaying execution in RIOT is usually done using xtimer_msleep().
         * However, to get this tiny example running even when no timer drivers
         * are written yet, we just use a CPU delay loop. We use the qualifier
         * volatile on the counter variable to prevent the compiler from
         * optimizing the delay loop and assume that something in the order of
         * 20 CPU cycles is needed for one loop iteration, so that we have
         * a delay that is roughly in the order of a second.
         */
        for (volatile uint32_t i = 0; i < CLOCK_CORECLOCK / 20; i++) { }
        LED0_TOGGLE;
    }
#endif

    return 0;
}
