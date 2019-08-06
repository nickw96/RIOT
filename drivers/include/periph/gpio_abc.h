/*
 * Copyright 2019 Marian Buschsieweke
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    drivers_periph_gpio_abc GPIO Advanced Bitbanging Capabilities (ABC)
 * @ingroup     drivers_periph_gpio
 * @brief       GPIO extension for precisely timed GPIO accesses
 *
 * Description
 * ===========
 *
 * This submodule extends @ref drivers_periph_gpio to allow precisely timed
 * GPIO accesses with a sub-microsecond resolution. This resolution cannot be
 * implemented by relying on timers, but instead require counting CPU cycles.
 * The goal of this extension is to provide a platform independent API that
 * allows to implement bit banging protocols with high data rates and tight
 * timing constraints in a portable manner.
 *
 * Accuracy
 * ========
 *
 * The accuracy of GPIO ABC depends on correct values for
 * @ref GPIO_ABC_LOOP_CYCLES and @ref GPIO_ABC_OVERHEAD_CYCLES, the accuracy
 * of the CPU frequency and how much overhead the code between the calls to
 * @ref gpio_set_for and @ref gpio_clear_for adds. The GPIO ABC extension will
 * warn during compilation if the expected accuracy is less than 150 nano
 * seconds.
 *
 * Adding Support for GPIO ABC
 * ===========================
 *
 * In order to add support for GPIO ABC to a platform, three things need to
 * be provided:
 *
 * 1. Implementations of @ref gpio_set_for and @ref gpio_clear_for
 * 2. The number of CPU cycles one iteration of the delay loop takes defined as
 *    @ref GPIO_ABC_LOOP_CYCLES
 * 3. The number of CPU cycles one call to @ref gpio_set_for or
 *    @ref gpio_clear_for takes defined as @ref  GPIO_ABC_OVERHEAD_CYCLES
 *
 * Implementation Hints
 * --------------------
 *
 * The implementations of @ref gpio_set_for and @ref gpio_clear_for have to
 * inline the logic to set/clear the GPIO pin and the delay loop instead of
 * calling @ref gpio_set and @ref gpio_clear. Otherwise the overhead of the
 * function calls would prevent sending short pulses. The logic for
 * setting/clearing the pin can (and should) be implemented in C. This logic
 * is rather simple and therefore has little potential for compiler
 * optimizations and, thus, will take about the same time independent of the
 * compiler. The delay loop however has to be implemented in inline assembly,
 * as even one CPU cycle more or less accumulates over each spin of the loop.
 *
 * On more sophisticated platforms with features like dynamic branch prediction,
 * special care needs to be taken that each spin of the delay loop requires
 * the same number of CPU cycles.
 *
 * Determination of Overhead Cycles and Loop Cycles
 * ------------------------------------------------
 *
 * On simple platforms one could simple look up the number of CPU cycles each
 * instruction in the delay loop takes and sum up. On more sophisticated
 * platforms many aspects need to be taken into account, like length of the
 * instruction pipeline, pipeline stalls, width of the bus connected to the
 * flash, alignment of instructions, etc. Often it is just faster to start with
 * a wild guess and use the application in `tests/periph_gpio_abc` to measure
 * and adjust the values until the test passes. You will need an oscilloscope
 * or a logic analyzer (with at least 20 MHz sample rate) to measure the length
 * of the GPIO pulses generated.
 *
 * @{
 * @file
 * @brief       GPIO Advanced Bitbanging Capabilities (ABC)
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef PERIPH_GPIO_ABC_H
#define PERIPH_GPIO_ABC_H

#include "board.h"
#include "cpu_gpio_abc.h"
#include "periph/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   The minimum accuracy a GPIO ABC implementation has to provide in nano seconds
 */
#define GPIO_ABC_MIN_ACCURACY       (150U)

#ifndef GPIO_ABC_OVERHEAD_NS
/**
 * @brief   Overhead to compensate for in delay loop measured in nano seconds
 */
#define GPIO_ABC_OVERHEAD_NS        ((1000000000ULL * GPIO_ABC_OVERHEAD_CYCLES) / CLOCK_CORECLOCK)
#endif /* GPIO_ABC_MIN_ACCURACY */

/**
 * @brief   Minimum pulse length (in nanosecond) supported by this backend
 */
#define GPIO_ABC_MIN_PULSE_LEN      GPIO_ABC_OVERHEAD_NS

/**
 * @brief   Expected accuracy of the timer in nanoseconds
 *
 * This assumes that the parameter for overhead and cycles per loop are 100%
 * correct, but the desired delay would be achieved just in the middle of
 * a delay loop iteration (worst case). The value is rounded up.
 */
#define GPIO_ABC_ACCURACY_NS        ((1000000000ULL * (GPIO_ABC_LOOP_CYCLES + 1) - 1) / (CLOCK_CORECLOCK * 2))

#if GPIO_ABC_ACCURACY_NS > GPIO_ABC_MIN_ACCURACY
#warning "The GPIO ABC implementation has an accuracy of less than 150ns"
#endif

/**
 * @brief   Calculates the delay parameter from a pulse length in nano seconds
 *
 * This macro can be used instead of @ref gpio_abc_delay when the pulse length
 * is a compile time constant. When doing so, please check against
 * @ref GPIO_ABC_MIN_PULSE_LEN if the required pulse length is not too short
 * for the GPIO ABC implementation.
 */
#define GPIO_ABC_DELAY(ns) \
    ((int)( \
        ((ns) < GPIO_ABC_OVERHEAD_NS) ? 0 : \
        ((uint64_t)(ns) - GPIO_ABC_OVERHEAD_NS) * CLOCK_CORECLOCK / (1000000000ULL * GPIO_ABC_LOOP_CYCLES)) \
    )

/**
 * @brief   Calculate the delay parameter for precise timing
 *
 * @param   ns      Intended pulse length in nano seconds
 *
 * @return  The delay parameter to pass to @ref gpio_set_for and @ref gpio_clear_for
 * @retval  -1      The pulse length is too short to be feasible
 *
 * @details If the delay is a compile time constant, use @ref GPIO_ABC_DLEAY instead
 */
int gpio_abc_delay(uint16_t ns);

/**
 * @brief   Set the given pin to HIGH and wait for the given duration
 *
 * @param[in] pin       The pin to set
 * @param[in] delay     Number of delay loop iterations calculated with @p gpio_abc_delay
 *
 * @post    The GPIO pin identified by @p pin has been set and afterwards a
 *          delay loop spun for @p delay iterations
 *
 * @details If @p delay is zero or negative, this function returns as soon as possible
 */
void gpio_set_for(gpio_t pin, int delay);

/**
 * @brief   Set the given pin to LOW and wait for the given duration
 *
 * @param[in] pin       The pin to clear
 * @param[in] delay     Number of delay loop iterations calculated with @p gpio_abc_delay
 *
 * @post    The GPIO pin identified by @p pin has been cleared and afterwards a
 *          delay loop spun for @p delay iterations
 *
 * @details If @p delay is zero or negative, this function returns as soon as possible
 */
void gpio_clear_for(gpio_t pin, int delay);

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_GPIO_ABC_H */
/** @} */
