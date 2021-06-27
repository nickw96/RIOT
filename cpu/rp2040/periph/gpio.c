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

<<<<<<< HEAD
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
=======
#include "bitarithm.h"
#include "board.h"
#include "irq.h"
#include "periph/gpio.h"
#include "periph_conf.h"
#include "periph_cpu.h"
#include "reg_atomic.h"

#define ENABLE_DEBUG    0
#include "debug.h"

#define GPIO_PIN_NUMOF  30U

#ifdef MODULE_PERIPH_GPIO_IRQ
static gpio_cb_t _cbs[GPIO_PIN_NUMOF];
static void *_args[GPIO_PIN_NUMOF];
#endif /* MODULE_PERIPH_GPIO_IRQ */

int gpio_init(gpio_t pin, gpio_mode_t mode)
{
    assert(pin < GPIO_PIN_NUMOF);
>>>>>>> fd7c4071a8cbdc2f3c17b4985db2ebdee52bbec7
    volatile gpio_pad_ctrl_t *pad_config_reg = gpio_pad_register(pin);
    volatile gpio_io_ctrl_t *io_config_reg = gpio_io_register(pin);
    SIO->GPIO_OE_CLR.reg = 1LU << pin;
    SIO->GPIO_OUT_CLR.reg = 1LU << pin;

<<<<<<< HEAD
    if(gpio_mode_mask & (1LU << pin))
        gpio_mode_mask -= 1LU << pin;

=======
>>>>>>> fd7c4071a8cbdc2f3c17b4985db2ebdee52bbec7
    switch (mode) {
    case GPIO_IN:
        {
            gpio_pad_ctrl_t pad_config = {
<<<<<<< HEAD
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
=======
                .input_enable = 1,
            };
            gpio_io_ctrl_t io_config = {
                .function_select = FUNCTION_SELECT_SIO,
>>>>>>> fd7c4071a8cbdc2f3c17b4985db2ebdee52bbec7
            };
            *pad_config_reg = pad_config;
            *io_config_reg = io_config;
        }
        break;
    case GPIO_IN_PD:
        {
            gpio_pad_ctrl_t pad_config = {
<<<<<<< HEAD
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
=======
                .pull_down_enable = 1,
                .input_enable = 1,
            };
            gpio_io_ctrl_t io_config = {
                .function_select = FUNCTION_SELECT_SIO,
>>>>>>> fd7c4071a8cbdc2f3c17b4985db2ebdee52bbec7
            };
            *pad_config_reg = pad_config;
            *io_config_reg = io_config;
        }
        break;
    case GPIO_IN_PU:
        {
            gpio_pad_ctrl_t pad_config = {
<<<<<<< HEAD
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
=======
                .pull_up_enable = 1,
                .input_enable = 1,
            };
            gpio_io_ctrl_t io_config = {
                .function_select = FUNCTION_SELECT_SIO,
>>>>>>> fd7c4071a8cbdc2f3c17b4985db2ebdee52bbec7
            };
            *pad_config_reg = pad_config;
            *io_config_reg = io_config;
        }
        break;
    case GPIO_OUT:
        {
            gpio_pad_ctrl_t pad_config = {
<<<<<<< HEAD
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
=======
                .drive_strength = DRIVE_STRENGTH_12MA,
            };
            gpio_io_ctrl_t io_config = {
                .function_select = FUNCTION_SELECT_SIO,
>>>>>>> fd7c4071a8cbdc2f3c17b4985db2ebdee52bbec7
            };
            *pad_config_reg = pad_config;
            *io_config_reg = io_config;
        }
        SIO->GPIO_OE_SET.reg = 1LU << pin;
<<<<<<< HEAD
        gpio_mode_mask |= 1LU << pin;
=======
>>>>>>> fd7c4071a8cbdc2f3c17b4985db2ebdee52bbec7
        break;
    default:
        return -ENOTSUP;
    }
    return 0;
}

int gpio_read(gpio_t pin)
{
<<<<<<< HEAD
    if(gpio_mode_mask & (1LU << pin))
        return (SIO->GPIO_OUT.reg & (1LU << pin));
    else
        return (SIO->GPIO_IN.reg & (1LU << pin));
=======
    if (SIO->GPIO_OE.reg & (1LU << pin)) {
        /* pin is output: */
        return SIO->GPIO_OUT.reg & (1LU << pin);
    }
    /* pin is input: */
    return SIO->GPIO_IN.reg & (1LU << pin);
>>>>>>> fd7c4071a8cbdc2f3c17b4985db2ebdee52bbec7
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
<<<<<<< HEAD
=======

#ifdef MODULE_PERIPH_GPIO_IRQ
static void _irq_enable(unsigned pin, unsigned flank)
{
    volatile uint32_t *int_enable_regs = &IO_BANK0->PROC0_INTE0.reg;
    unsigned shift_amount = (pin & 0x7) << 2;
    unsigned idx = pin >> 3;
    /* make access atomic by disabling IRQs */
    unsigned irq_state = irq_disable();
    uint32_t value = int_enable_regs[idx];
    /* first, clear previous setting */
    value &= ~(0xFLU << shift_amount);
    /* then, apply new setting */
    value |= flank << shift_amount;
    int_enable_regs[idx] = value;
    irq_restore(irq_state);
    NVIC_EnableIRQ(IO_IRQ_BANK0_IRQn);
}

void gpio_irq_enable(gpio_t pin)
{
    reg_atomic_clear(gpio_io_register_u32(pin), IO_BANK0_GPIO1_CTRL_IRQOVER_Msk);
}

void gpio_irq_disable(gpio_t pin)
{
    /* Beware: The two-bit IRQOVER value needs to be set to 0b10 == IRQ_OVERRIDE_LOW. This
     * implementation will set IRQOVER only to either 0b00 == IRQ_OVERRIDE_NORMAL or
     * 0b10 == IRQ_OVERRIDE_LOW. If we just set the most significant bit, this will result in
     * IRQOVER set to IRQ_OVERRIDE_LOW.
     *
     * IRQOVER must not be set by user code for this to work, though.
     */
    reg_atomic_set(gpio_io_register_u32(pin), IRQ_OVERRIDE_LOW << IO_BANK0_GPIO1_CTRL_IRQOVER_Pos);
}

int gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank, gpio_cb_t cb, void *arg)
{
    assert(pin < GPIO_PIN_NUMOF);
    int retval = gpio_init(pin, mode);
    if (retval) {
        return retval;
    }

    _cbs[pin] = cb;
    _args[pin] = arg;
    _irq_enable(pin, flank);

    return 0;
}

void isr_io_bank0(void)
{
    unsigned offset = 0;
    volatile uint32_t *irq_status_regs = &IO_BANK0->PROC0_INTS0.reg;
    volatile uint32_t *irq_ack_regs = &IO_BANK0->INTR0.reg;

    for (unsigned i = 0; i < (GPIO_PIN_NUMOF + 7) / 8; i++, offset += 8) {
        unsigned status = irq_status_regs[i];
        irq_ack_regs[i] = status;
        for (unsigned pin = 0; pin < 8; pin++) {
            if (status & (0xFLU << (pin << 2))) {
                if (_cbs[pin + offset]) {
                    _cbs[pin + offset](_args[pin + offset]);
                }
            }
        }
    }
}

#endif /* MODULE_PERIPH_GPIO_IRQ */
>>>>>>> fd7c4071a8cbdc2f3c17b4985db2ebdee52bbec7
