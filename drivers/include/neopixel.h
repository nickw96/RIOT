/*
 * Copyright 2019 Marian Buschsieweke
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    drivers_neopixel WS2812/SK6812 RGB LED (NeoPixel)
 * @ingroup     drivers_actuators
 * @brief       Driver for the WS2812 or the SK6812 RGB LEDs sold as NeoPixel
 *
 * The WS2812 or SK6812 RGB LEDs, or more commonly known as NeoPixels, can be
 * chained so that a single data pin of the MCU can control an arbitrary number
 * of RGB LED.
 *
 * @{
 *
 * @file
 * @brief       WS2812/SK6812 RGB LED Driver
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <stdint.h>

#include "color.h"
#include "periph/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   The number of bytes to allocate in the data buffer per LED
 */
#define NEOPIXEL_BYTES_PER_DEVICE       (3U)

/**
 * @brief   Struct to hold initialization parameters for a NeoPixel RGB LED
 */
typedef struct {
    /**
     * @brief   A statically allocated data buffer storing the state of the LEDs
     *
     * @pre     Must be sized `numof * NEOPIXEL_BYTES_PER_DEVICE` bytes
     */
    uint8_t *buf;
    uint16_t numof;             /**< Number of chained RGB LEDs */
    gpio_t pin;                 /**< GPIO connected to the data pin of the first LED */
} neopixel_params_t;

/**
 * @brief   Device descriptor of a NeoPixel RGB LED chain
 */
typedef struct {
    neopixel_params_t params;   /**< Parameters of the LED chain */
} neopixel_t;

/**
 * @brief   Initialize an NeoPixel RGB LED chain
 *
 * @param   dev     Device descriptor to initialize
 * @param   params  Parameters to initialize the device with
 *
 * @retval  0       Success
 * @retval  -EINVAL Invalid argument
 * @retval  -EIO    Failed to initialize the data GPIO pin
 */
int neopixel_init(neopixel_t *dev, const neopixel_params_t *params);

/**
 * @brief   Sets the color of an LED in the chain in the internal buffer
 *
 * @param   dev     Device descriptor of the LED chain to modify
 * @param   index   The index of the LED to set the color of
 * @param   color   The new color to apply
 *
 * @warning This change will not become active until @ref neopixel_write is called
 */
void neopixel_set(neopixel_t *dev, uint16_t index, color_rgb_t color);

void neopixel_write(neopixel_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* NEOPIXEL_H */
/** @} */
