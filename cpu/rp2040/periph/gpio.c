/*
 * Copyright (C) 2021 Otto-von-Guericke Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_rp2040
 * @ingroup     drivers_periph_gpio
 * @{
 *
 * @file
 * @brief       GPIO driver implementation for the RP2040
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <errno.h>

#include "board.h"
#include "bitarithm.h"
#include "periph/gpio.h"
#include "periph_cpu.h"
#include "periph_conf.h"

#define ENABLE_DEBUG 0
#include "debug.h"

/**
 *  @brief  Mask for which GPIO pin uses GPIO_IN or GPIO_OUT
 */
uint32_t gpio_mode_mask = 0LU;

int gpio_init(gpio_t pin, gpio_mode_t mode)
{
    volatile gpio_pad_ctrl_t *pad_config_reg = gpio_pad_register(pin);
    volatile gpio_io_ctrl_t *io_config_reg = gpio_io_register(pin);
    SIO->GPIO_OE_CLR.reg = 1LU << pin;
    SIO->GPIO_OUT_CLR.reg = 1LU << pin;

    if(gpio_mode_mask & (1LU << pin))
        gpio_mode_mask -= 1LU << pin;

    switch (mode) {
    case GPIO_IN:
        {
            gpio_pad_ctrl_t pad_config = {
                .slew_rate_fast = 0,
                .schmitt_trig_enable = 0,
                .pull_down_enable = 0,
                .pull_up_enable = 0,
                .drive_strength = DRIVE_STRENGTH_2MA,
                .input_enable = 1,
                .output_disable = 0
            };
            gpio_io_ctrl_t io_config = {
                .function_select = FUNCTION_SELECT_SIO,
                .output_overide = OUTPUT_OVERRIDE_NORMAL,
                .output_enable_overide = OUTPUT_ENABLE_OVERRIDE_NOMARL,
                .input_override = INPUT_OVERRIDE_NOMARL,
                .irq_override = IRQ_OVERRIDE_NOMARL,
            };
            *pad_config_reg = pad_config;
            *io_config_reg = io_config;
        }
        break;
    case GPIO_IN_PD:
        {
            gpio_pad_ctrl_t pad_config = {
                .slew_rate_fast = 0,
                .schmitt_trig_enable = 0,
                .pull_down_enable = 1,
                .pull_up_enable = 0,
                .drive_strength = DRIVE_STRENGTH_2MA,
                .input_enable = 1,
                .output_disable = 0
            };
            gpio_io_ctrl_t io_config = {
                .function_select = FUNCTION_SELECT_SIO,
                .output_overide = OUTPUT_OVERRIDE_NORMAL,
                .output_enable_overide = OUTPUT_ENABLE_OVERRIDE_NOMARL,
                .input_override = INPUT_OVERRIDE_NOMARL,
                .irq_override = IRQ_OVERRIDE_NOMARL,
            };
            *pad_config_reg = pad_config;
            *io_config_reg = io_config;
        }
        break;
    case GPIO_IN_PU:
        {
            gpio_pad_ctrl_t pad_config = {
                .slew_rate_fast = 0,
                .schmitt_trig_enable = 0,
                .pull_down_enable = 0,
                .pull_up_enable = 1,
                .drive_strength = DRIVE_STRENGTH_2MA,
                .input_enable = 1,
                .output_disable = 0
            };
            gpio_io_ctrl_t io_config = {
                .function_select = FUNCTION_SELECT_SIO,
                .output_overide = OUTPUT_OVERRIDE_NORMAL,
                .output_enable_overide = OUTPUT_ENABLE_OVERRIDE_NOMARL,
                .input_override = INPUT_OVERRIDE_NOMARL,
                .irq_override = IRQ_OVERRIDE_NOMARL,
            };
            *pad_config_reg = pad_config;
            *io_config_reg = io_config;
        }
        break;
    case GPIO_OUT:
        {
            gpio_pad_ctrl_t pad_config = {
                .slew_rate_fast = 0,
                .schmitt_trig_enable = 0,
                .pull_down_enable = 0,
                .pull_up_enable = 0,
                .drive_strength = DRIVE_STRENGTH_2MA,
                .input_enable = 1,
                .output_disable = 0
            };
            gpio_io_ctrl_t io_config = {
                .function_select = FUNCTION_SELECT_SIO,
                .output_overide = OUTPUT_OVERRIDE_NORMAL,
                .output_enable_overide = OUTPUT_ENABLE_OVERRIDE_NOMARL,
                .input_override = INPUT_OVERRIDE_NOMARL,
                .irq_override = IRQ_OVERRIDE_NOMARL,
            };
            *pad_config_reg = pad_config;
            *io_config_reg = io_config;
        }
        SIO->GPIO_OE_SET.reg = 1LU << pin;
        gpio_mode_mask |= 1LU << pin;
        break;
    default:
        return -ENOTSUP;
    }
    return 0;
}

int gpio_read(gpio_t pin)
{
    if(gpio_mode_mask & (1LU << pin))
        return (SIO->GPIO_OUT.reg & (1LU << pin));
    else
        return (SIO->GPIO_IN.reg & (1LU << pin));
}

void gpio_set(gpio_t pin)
{
    SIO->GPIO_OUT_SET.reg = 1LU << pin;
}

void gpio_clear(gpio_t pin)
{
    SIO->GPIO_OUT_CLR.reg = 1LU << pin;
}

void gpio_toggle(gpio_t pin)
{
    SIO->GPIO_OUT_XOR.reg = 1LU << pin;
}

void gpio_write(gpio_t pin, int value)
{
    if (value) {
        gpio_set(pin);
    }
    else {
        gpio_clear(pin);
    }
}
