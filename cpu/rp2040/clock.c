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
 * @brief       Implementation of the CPU clock configuration
 *
 * @author      Fabian Hüßler <fabian.huessler@ovgu.de>
 *
 * @}
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "vendor/RP2040.h"
#include "vendor/system_RP2040.h"
#include "reg_atomic.h"

void clock_sys_configure_source(uint32_t f_in, uint32_t f_out, CLOCKS_CLK_SYS_CTRL_SRC_Enum source)
{
    assert(f_out <= f_in);
    assert(source != CLOCKS_CLK_SYS_CTRL_SRC_clksrc_clk_sys_aux);
    uint32_t div = (((uint64_t)f_in) << CLOCKS_CLK_SYS_DIV_INT_Pos) / f_out;
    /* switch the glitchless mux to clk_ref */
    CLOCKS->CLK_SYS_CTRL.bit.SRC = source;
    /* apply divider */
    CLOCKS->CLK_SYS_DIV.reg = div;
    /* poll SELECTED until the switch is completed */
    while (!(CLOCKS->CLK_SYS_SELECTED & (1U << source))) { }
    /* update SystemCoreClock variable */
    SystemCoreClockUpdate();
}

void clock_sys_configure_aux_source(uint32_t f_in, uint32_t f_out,
                                    CLOCKS_CLK_SYS_CTRL_AUXSRC_Enum aux)
{
    assert(f_out <= f_in);
    uint32_t div = (((uint64_t)f_in) << CLOCKS_CLK_SYS_DIV_INT_Pos) / f_out;
    /* switch the glitchless mux to a source that is not the aux mux */
    CLOCKS->CLK_SYS_CTRL.bit.SRC = CLOCKS_CLK_SYS_CTRL_SRC_clk_ref;
    /* poll SELECTED until the switch is completed */
    while (!(CLOCKS->CLK_SYS_SELECTED & (1U << CLOCKS_CLK_SYS_CTRL_SRC_clk_ref))) { }
    /* change the auxiliary mux */
    CLOCKS->CLK_SYS_CTRL.bit.AUXSRC = aux;
    /* apply divider */
    CLOCKS->CLK_SYS_DIV.reg = div;
    /* switch the glitchless mux to clk_sys_aux */
    CLOCKS->CLK_SYS_CTRL.bit.SRC = CLOCKS_CLK_SYS_CTRL_SRC_clksrc_clk_sys_aux;
    /* poll SELECTED until the switch is completed */
    while (!(CLOCKS->CLK_SYS_SELECTED & (1U << CLOCKS_CLK_SYS_CTRL_SRC_clksrc_clk_sys_aux))) { }
    /* update SystemCoreClock variable */
    SystemCoreClockUpdate();
}

void clock_ref_configure_source(uint32_t f_in, uint32_t f_out, CLOCKS_CLK_REF_CTRL_SRC_Enum source)
{
    assert(f_out <= f_in);
    assert(source != CLOCKS_CLK_REF_CTRL_SRC_clksrc_clk_ref_aux);
    uint32_t div = (((uint64_t)f_in) << CLOCKS_CLK_REF_DIV_INT_Pos) / f_out;
    /* switch the glitchless mux to clock source */
    CLOCKS->CLK_REF_CTRL.bit.SRC = source;
    /* apply divider */
    CLOCKS->CLK_REF_DIV.reg = div & CLOCKS_CLK_REF_DIV_INT_Msk;
    /* poll SELECTED until the switch is completed */
    while(!(CLOCKS->CLK_REF_SELECTED & (1U << source))) { }
}

void clock_ref_configure_aux_source(uint32_t f_in, uint32_t f_out,
                                    CLOCKS_CLK_REF_CTRL_AUXSRC_Enum aux)
{
    assert(f_out <= f_in);
    uint32_t div = (((uint64_t)f_in) << CLOCKS_CLK_REF_DIV_INT_Pos) / f_out;
    /* switch the glitchless mux to a source that is not the aux mux */
    CLOCKS->CLK_REF_CTRL.bit.SRC = CLOCKS_CLK_REF_CTRL_SRC_rosc_clksrc_ph;
    /* poll SELECTED until the switch is completed */
    while (!(CLOCKS->CLK_REF_SELECTED & (1U << CLOCKS_CLK_REF_CTRL_SRC_rosc_clksrc_ph))) { }
    /* change the auxiliary mux */
    CLOCKS->CLK_REF_CTRL.bit.AUXSRC = aux;
    /* apply divider */
    CLOCKS->CLK_REF_DIV.reg = div & CLOCKS_CLK_REF_DIV_INT_Msk;
    /* switch the glitchless mux to clk_ref_aux */
    CLOCKS->CLK_REF_CTRL.bit.SRC = CLOCKS_CLK_REF_CTRL_SRC_clksrc_clk_ref_aux;
    /* poll SELECTED until the switch is completed */
    while (!(CLOCKS->CLK_REF_SELECTED & (1U << CLOCKS_CLK_REF_CTRL_SRC_clksrc_clk_ref_aux))) { }
}

void clock_periph_configure(CLOCKS_CLK_PERI_CTRL_AUXSRC_Enum aux)
{
    CLOCKS->CLK_PERI_CTRL.bit.AUXSRC = aux;
    reg_atomic_set(&CLOCKS->CLK_PERI_CTRL.reg, (1u << CLOCKS_CLK_ADC_CTRL_ENABLE_Pos));
}

void clock_gpout0_configure(uint32_t f_in, uint32_t f_out, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_Enum aux)
{
    assert(f_out <= f_in);
    uint32_t div = (((uint64_t)f_in) << CLOCKS_CLK_REF_DIV_INT_Pos) / f_out;
    CLOCKS->CLK_GPOUT0_CTRL.bit.AUXSRC = aux;
    CLOCKS->CLK_GPOUT0_DIV.reg = div;
    reg_atomic_set(&CLOCKS->CLK_GPOUT0_CTRL.reg, 1U << CLOCKS_CLK_GPOUT0_CTRL_ENABLE_Pos);
    reg_atomic_set(&PADS_BANK0->GPIO21.reg, 1U << PADS_BANK0_GPIO21_IE_Pos);
    IO_BANK0->GPIO21_CTRL.bit.FUNCSEL = IO_BANK0_GPIO21_CTRL_FUNCSEL_clocks_gpout_0;
}

void clock_gpout1_configure(uint32_t f_in, uint32_t f_out, CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_Enum aux)
{
    assert(f_out <= f_in);
    uint32_t div = (((uint64_t)f_in) << CLOCKS_CLK_REF_DIV_INT_Pos) / f_out;
    CLOCKS->CLK_GPOUT1_CTRL.bit.AUXSRC = aux;
    CLOCKS->CLK_GPOUT1_DIV.reg = div;
    reg_atomic_set(&CLOCKS->CLK_GPOUT1_CTRL.reg, 1U << CLOCKS_CLK_GPOUT1_CTRL_ENABLE_Pos);
    reg_atomic_set(&PADS_BANK0->GPIO23.reg, 1U << PADS_BANK0_GPIO23_IE_Pos);
    IO_BANK0->GPIO23_CTRL.bit.FUNCSEL = IO_BANK0_GPIO23_CTRL_FUNCSEL_clocks_gpout_1;
}

void clock_gpout2_configure(uint32_t f_in, uint32_t f_out, CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_Enum aux)
{
    assert(f_out <= f_in);
    uint32_t div = (((uint64_t)f_in) << CLOCKS_CLK_REF_DIV_INT_Pos) / f_out;
    CLOCKS->CLK_GPOUT2_CTRL.bit.AUXSRC = aux;
    CLOCKS->CLK_GPOUT2_DIV.reg = div;
    reg_atomic_set(&CLOCKS->CLK_GPOUT2_CTRL.reg, 1U << CLOCKS_CLK_GPOUT2_CTRL_ENABLE_Pos);
    reg_atomic_set(&PADS_BANK0->GPIO24.reg, 1U << PADS_BANK0_GPIO24_IE_Pos);
    IO_BANK0->GPIO24_CTRL.bit.FUNCSEL = IO_BANK0_GPIO24_CTRL_FUNCSEL_clocks_gpout_2;
}

void clock_gpout3_configure(uint32_t f_in, uint32_t f_out, CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_Enum aux)
{
    assert(f_out <= f_in);
    uint32_t div = (((uint64_t)f_in) << CLOCKS_CLK_REF_DIV_INT_Pos) / f_out;
    CLOCKS->CLK_GPOUT3_CTRL.bit.AUXSRC = aux;
    CLOCKS->CLK_GPOUT3_DIV.reg = div;
    reg_atomic_set(&CLOCKS->CLK_GPOUT3_CTRL.reg, 1U << CLOCKS_CLK_GPOUT3_CTRL_ENABLE_Pos);
    reg_atomic_set(&PADS_BANK0->GPIO25.reg, 1U << PADS_BANK0_GPIO25_IE_Pos);
    IO_BANK0->GPIO25_CTRL.bit.FUNCSEL = IO_BANK0_GPIO25_CTRL_FUNCSEL_clocks_gpout_3;
}
