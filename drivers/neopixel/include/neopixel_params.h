/*
 * Copyright (C) 2019 Marian Buschsieweke
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_neopixel
 *
 * @{
 * @file
 * @brief       Default configuration for WS2812/SK6812 RGB LEDs
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef NEOPIXEL_PARAMS_H
#define NEOPIXEL_PARAMS_H

#include "board.h"
#include "neopixel.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Default configuration parameters for NeoPixel RGB LEDs
 * @{
 */
#ifndef NEOPIXEL_PARAM_PIN
#define NEOPIXEL_PARAM_PIN              (GPIO_PIN(0,0)) /**< GPIO pin connected to the data pin of the first LED */
#endif
#ifndef NEOPIXEL_PARAM_NUMOF
#define NEOPIXEL_PARAM_NUMOF            (8U)            /**< Number of LEDs chained */
#endif
#ifndef NEOPIXEL_PARAM_BUF
extern uint8_t neopixel_buf[NEOPIXEL_PARAM_NUMOF * NEOPIXEL_BYTES_PER_DEVICE];
#define NEOPIXEL_PARAM_BUF              (neopixel_buf)  /**< Data buffer holding LED states */
#endif

#ifndef NEOPIXEL_PARAMS
#define NEOPIXEL_PARAMS                 { \
                                            .pin = NEOPIXEL_PARAM_PIN,  \
                                            .numof = NEOPIXEL_PARAM_NUMOF, \
                                            .buf = NEOPIXEL_PARAM_BUF, \
                                        }
#endif
/**@}*/

/**
 * @brief   Initialization parameters for NeoPixel device descriptors
 */
static const neopixel_params_t neopixel_params[] =
{
    NEOPIXEL_PARAMS
};

#ifdef __cplusplus
}
#endif

#endif /* NEOPIXEL_PARAMS_H */
/** @} */
