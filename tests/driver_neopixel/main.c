/*
 * Copyright 2019 Marian Buschsieweke
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief   Test application for the NeoPixel RGB LED driver
 *
 * @author  Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <stdio.h>

#include "neopixel.h"
#include "neopixel_params.h"
#include "xtimer.h"

static const color_rgb_t rainbow[] = {
    {.r = 0x94, .g = 0x00, .b = 0xd3},
    {.r = 0x4b, .g = 0x00, .b = 0x82},
    {.r = 0x00, .g = 0x00, .b = 0xff},
    {.r = 0x00, .g = 0xff, .b = 0x00},
    {.r = 0xff, .g = 0xff, .b = 0x00},
    {.r = 0xff, .g = 0x7f, .b = 0xd3},
    {.r = 0xff, .g = 0x00, .b = 0x00},
};

#define RAINBOW_LEN     (sizeof(rainbow) / sizeof(rainbow[0]))

int main(void)
{
    neopixel_t dev;
    int retval;

    puts(
        "NeoPixel test application\n"
        "=========================\n"
        "\n"
        "If you see an animated rainbow, the driver works as expected.\n"
        "If the LEDs are flickering, check if the power supply is sufficient\n"
        "(at least 4V). Also: The logic level has to be at least 0.7 * VDD,\n"
        "so 3.3V logic with a 5V power supply is out of spec, but might work\n"
        "OK.\n"
    );

    if (0 != (retval = neopixel_init(&dev, &neopixel_params[0]))) {
        printf("Initialization failed with error code %d\n", retval);
        return retval;
    }

    while (1) {
        unsigned offset = 0;
        puts("Animation: Moving rainbow...");
        for (unsigned i = 0; i < 100; i++) {
            for (uint16_t j = 0; j < dev.params.numof; j++) {
                neopixel_set(&dev, j, rainbow[(j + offset) % RAINBOW_LEN]);
            }
            offset++;
            neopixel_write(&dev);
            xtimer_usleep(100 * US_PER_MS);
        }

        puts("Animation: Fading rainbow...");
        for (unsigned i = 0; i < RAINBOW_LEN; i++) {
            for (unsigned j = 0; j < 100; j++) {
                color_rgb_t col = {
                    .r = (uint8_t)(((unsigned)rainbow[i].r * j + 50) / 100),
                    .g = (uint8_t)(((unsigned)rainbow[i].g * j + 50) / 100),
                    .b = (uint8_t)(((unsigned)rainbow[i].b * j + 50) / 100),
                };
                for (uint16_t k = 0; k < dev.params.numof; k++) {
                    neopixel_set(&dev, k, col);
                }
                neopixel_write(&dev);
                xtimer_usleep(10 * US_PER_MS);
            }
            for (unsigned j = 100; j > 0; j--) {
                color_rgb_t col = {
                    .r = (uint8_t)(((unsigned)rainbow[i].r * j + 50) / 100),
                    .g = (uint8_t)(((unsigned)rainbow[i].g * j + 50) / 100),
                    .b = (uint8_t)(((unsigned)rainbow[i].b * j + 50) / 100),
                };
                for (uint16_t k = 0; k < dev.params.numof; k++) {
                    neopixel_set(&dev, k, col);
                }
                neopixel_write(&dev);
                xtimer_usleep(10 * US_PER_MS);
            }
        }
    }

    return 0;
}
