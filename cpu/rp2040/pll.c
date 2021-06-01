/*
 * Copyright (C) 2021 Otto-von-Guericke Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_rp2040
 * @{
 *
 * @file
 * @brief       Implementation of the phase locked loop (PLL)
 *              to drive the ROSC or XOSC to a higher frequency
 *
 * @author      Fabian Hüßler <fabian.huessler@ovgu.de>
 *
 * @}
 */

#include <assert.h>

#include "macros/units.h"
#include "periph_cpu.h"
#include "vendor/RP2040.h"
#include "reg_atomic.h"

#define RP2040_REF_CLOCK_MIN        (MHZ(5U))
#define RP2040_REF_CLOCK_MAX        (MHZ(15U))
#define RP2040_REF_CLOCK_DIV_MIN    (1U)
#define RP2040_REF_CLOCK_DIV_MAX    (63U)
#define RP2040_VCO_FEEDBACK_DIV_MIN (16U)
#define RP2040_VCO_FEEDBACK_DIV_MAX (320U)
#define RP2040_POST_DIV_MIN         (1U)
#define RP2040_POST_DIV_MAX         (7U)

/**
 * @brief   Start the PLL for the system clock (@p pll = PLL_SYS) or
 *          the USB clock (@p pll = PLL_USB)
 */
static void _pll_start(PLL_SYS_Type *pll,
                       uint32_t f_ref, uint8_t ref_div,
                       uint16_t vco_feedback_scale,
                       uint8_t post_div_1, uint8_t post_div_2)
{
    assert(RP2040_REF_CLOCK_MIN <= f_ref);
    assert(f_ref <= RP2040_REF_CLOCK_MAX);
    assert(RP2040_REF_CLOCK_DIV_MIN <= ref_div);
    assert(ref_div <= RP2040_REF_CLOCK_DIV_MAX);
    assert(RP2040_VCO_FEEDBACK_DIV_MIN <= vco_feedback_scale);
    assert(vco_feedback_scale <= RP2040_VCO_FEEDBACK_DIV_MAX);
    assert(RP2040_POST_DIV_MIN <= post_div_1);
    assert(post_div_1 <= RP2040_POST_DIV_MAX);
    assert(RP2040_POST_DIV_MIN <= post_div_2);
    assert(post_div_2 <= RP2040_POST_DIV_MAX);

    /* program reference clock divider */
    pll->CS.bit.REFDIV = ref_div;
    /* program feedback divider */
    pll->FBDIV_INT.bit.FBDIV_INT = vco_feedback_scale;
    /* turn on the main power */
    reg_atomic_clear(&pll->PWR.reg,
                     (1U << PLL_SYS_PWR_VCOPD_Pos) |
                     (1U << PLL_SYS_PWR_DSMPD_Pos) |
                     (1U << PLL_SYS_PWR_PD_Pos));
    /* wait for VCO to lock (i.e. keep its output stable) */
    while (!pll->CS.bit.LOCK) { }
    /* set up post divisors and turn them on */
    pll->PRIM.bit.POSTDIV1 = post_div_1;
    pll->PRIM.bit.POSTDIV2 = post_div_2;
    reg_atomic_clear(&pll->PWR.reg, 1U << PLL_SYS_PWR_POSTDIVPD_Pos);
}

/**
 * @brief   Stop the PLL for the system clock (@p pll = PLL_SYS) or
 *          the USB clock (@p pll = PLL_USB)
 */
static void _pll_stop(PLL_SYS_Type *pll)
{
    uint32_t reg = (1U << PLL_SYS_PWR_VCOPD_Pos) |
                   (1U << PLL_SYS_PWR_POSTDIVPD_Pos) |
                   (1U << PLL_SYS_PWR_DSMPD_Pos) |
                   (1U << PLL_SYS_PWR_PD_Pos);
    reg_atomic_set(&pll->PWR.reg, reg);
}

void pll_start_sys(uint32_t f_ref, uint8_t ref_div,
                   uint16_t vco_feedback_scale,
                   uint8_t post_div_1, uint8_t post_div_2)
{
    _pll_start(PLL_SYS, f_ref, ref_div, vco_feedback_scale, post_div_1, post_div_2);
}

void pll_start_usb(uint32_t f_ref, uint8_t ref_div,
                   uint16_t vco_feedback_scale,
                   uint8_t post_div_1, uint8_t post_div_2)
{
    _pll_start(PLL_USB, f_ref, ref_div, vco_feedback_scale, post_div_1, post_div_2);
}

void pll_stop_sys(void)
{
    _pll_stop(PLL_SYS);
}

void pll_stop_usb(void)
{
    _pll_stop(PLL_USB);
}

/**
 * @brief   Reset the PLL of the system clock
 */
void pll_reset_sys(void)
{
    periph_reset(1U << RESETS_RESET_pll_sys_Pos);
    periph_reset_done(1U << RESETS_RESET_pll_sys_Pos);
}

/**
 * @brief   Reset the PLL of the USB clock
 */
void pll_reset_usb(void)
{
    periph_reset(1U << RESETS_RESET_pll_usb_Pos);
    periph_reset_done(1U << RESETS_RESET_pll_usb_Pos);
}
