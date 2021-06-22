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

    gpio_t led_in = LED0_PIN;
    gpio_t led_pu = GPIO_PIN(0,4);
    gpio_t led_pd = GPIO_PIN(0,5);
    gpio_t button_in = GPIO_PIN(0,22);
    gpio_t button_pu = GPIO_PIN(0,21);
    gpio_t button_pd = GPIO_PIN(0,20);

    int button_in_state = 0;
    int button_pu_state = 0;
    int button_pd_state = 0;

    if (!gpio_init(led_in, GPIO_OUT) &&
     !gpio_init(button_in, GPIO_IN) && 
     !gpio_init(button_pu, GPIO_IN_PU) && 
     !gpio_init(button_pd, GPIO_IN_PD)  &&
     !gpio_init(led_pu, GPIO_OUT) &&
     !gpio_init(led_pd, GPIO_OUT)) {
        while (1) {
            button_in_state = gpio_read(button_in);
            button_pu_state = gpio_read(button_pu);
            button_pd_state = gpio_read(button_pd);
            for (volatile uint32_t i = 0; i < 1250000; i++) {}
            if (button_in_state > 0)
                gpio_toggle(led_in);
            if (button_pu_state > 0)
                gpio_toggle(led_pu);
            if (button_pd_state == 0)
                gpio_toggle(led_pd);
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
