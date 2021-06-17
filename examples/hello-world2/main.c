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
#include "../../drivers/include/periph/gpio.h"

int main(void)
{
    puts("Hello World!");

    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    gpio_t led = LED0_PIN;

    int led_state = 0;

    if (!gpio_init(led, GPIO_OUT)) {
        while (1) {
            led_state = gpio_read(led);
            for (volatile uint32_t i = 0; i < 1250000; i++) {}
            if(led_state == 0) {
                gpio_write(led, 1);
            }
            else {
                gpio_write(led, 0);
            }
        }
    }


/*
#ifdef LED0_TOGGLE
    while (1) {
        for (volatile uint32_t i = 0; i < 1250000; i++) { }
        LED0_TOGGLE;
    }
#endif
*/
    return 0;
}
