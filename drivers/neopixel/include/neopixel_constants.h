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
 * @brief       Constants for WS2812/SK6812 RGB LEDs
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef NEOPIXEL_CONSTANTS_H
#define NEOPIXEL_CONSTANTS_H

#include "periph/gpio_abc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The high time to encode a zero must not be longer than 450ns
 * (300ns + 150ns tolerance on the SK6812), otherwise it will be confused with
 * a one.
 */
#if GPIO_ABC_MIN_PULSE_LEN > 450
#error "NeoPixel: GPIO ABC is unable to generate short enough pulses"
#endif

/*
 * In order to support both variants, the timing tolerance is only 100ns instead
 * of 150ns, see explanation below
 */
#if GPIO_ABC_ACCURACY_NS > 100
#error "NeoPixel: GPIO ABC is unable to provide the accuracy required"
#endif

/**
 * @name    Timing parameters for NeoPixel RGB LEDs
 * @{
 */
/**
 * @brief   Time in nanoseconds to pull the data line high when encoding a zero
 *
 * For the WS2812 it is 350ns ± 150ns, for the SK6812 it is 300ns ± 150ns.
 * We choose 325ns to remain compatible with both (± 125ns tolerance).
 */
#define NEOPIXEL_DELAY_ZERO_HIGH        (GPIO_ABC_DELAY(325))
/**
 * @brief   Time in nanoseconds to pull the data line low when encoding a zero
 *
 * For the WS2812 it is 800ns ± 150ns, for the SK6812 it is 900ns ± 150ns.
 * We choose 850ns to remain compatible with both (± 100ns tolerance).
 */
#define NEOPIXEL_DELAY_ZERO_LOW         (GPIO_ABC_DELAY(850))
/**
 * @brief   Time in nanoseconds to pull the data line high when encoding a one
 *
 * For the WS2812 it is 700ns ± 150ns, for the SK6812 it is 600ns ± 150ns.
 * We choose 650ns to remain compatible with both (± 100ns tolerance).
 */
#define NEOPIXEL_DELAY_ONE_HIGH         (GPIO_ABC_DELAY(650))
/**
 * @brief   Time in nanoseconds to pull the data line low when encoding a one
 *
 * For both the WS2812 and the SK6812 it is 600ns ± 150ns.
 */
#define NEOPIXEL_DELAY_ONE_LOW          (GPIO_ABC_DELAY(600))
/**
 * @brief   Time in microseconds to pull the data line low to signal end of data
 *
 * For the WS2812 it is ≥ 50µs, for the SK6812 it is ≥ 80µs. We choose 80µs to
 * be compatible with both.
 */
#define NEOPIXEL_T_END_US               (80)
/**@}*/

/**
 * @name    Data encoding parameters for NeoPixel RGB LEDs
 * @{
 */
/**
 * @brief   Offset for the red color component
 */
#define NEOPIXEL_OFFSET_R               (1U)
/**
 * @brief   Offset for the green color component
 */
#define NEOPIXEL_OFFSET_G               (0U)
/**
 * @brief   Offset for the blue color component
 */
#define NEOPIXEL_OFFSET_B               (2U)
/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* NEOPIXEL_CONSTANTS_H */
/** @} */
