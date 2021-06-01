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
 * @brief       Implementation of the CPU initialization
 *
 * @author      Fabian Hüßler <fabian.huessler@ovgu.de>
 *
 * @}
 */

#include "cpu.h"
#include "macros/units.h"
#include "periph/init.h"
#include "periph_cpu.h"
#include "reg_atomic.h"
#include "stdio_base.h"
#include "vendor/RP2040.h"

static void _cpu_reset(void)
{
    /* 2.14 subsystem resets */
    uint32_t rst;
    /* Reset hardware components except for critical ones */
    rst = RESETS_RESET_MASK &
          ~(RESETS_RESET_usbctrl_Msk    |
            RESETS_RESET_syscfg_Msk     |
            RESETS_RESET_pll_usb_Msk    |
            RESETS_RESET_pll_sys_Msk    |
            RESETS_RESET_pads_qspi_Msk  |
            RESETS_RESET_io_qspi_Msk);
    periph_reset(rst);
    /* Assert that reset has completed except for those components which
       are not clocked by clk_ref or clk_sys */
    rst = RESETS_RESET_MASK &
          ~(RESETS_RESET_usbctrl_Msk    |
            RESETS_RESET_uart1_Msk      |
            RESETS_RESET_uart0_Msk      |
            RESETS_RESET_spi1_Msk       |
            RESETS_RESET_spi0_Msk       |
            RESETS_RESET_rtc_Msk        |
            RESETS_RESET_adc_Msk);
    periph_reset_done(rst);
    /* start XOSC running at 12 MHz */
    xosc_start(MHZ(12));
    /* reset system PLL */
    pll_reset_sys();
    /* make the system PLL output a 125 MHz frequency from the 12 MHz XOSC */
    pll_start_sys(MHZ(12), 1, 125, 6, 2);
    /* configure reference clock to run from 12 MHz XOSC */
    clock_ref_configure_source(MHZ(12), MHZ(12),
                               CLOCKS_CLK_REF_CTRL_SRC_xosc_clksrc);
    /* configure system clock output */
    clock_sys_configure_aux_source(MHZ(125), MHZ(125),
                                   CLOCKS_CLK_SYS_CTRL_AUXSRC_clksrc_pll_sys);
    /* configure the peripheral clock to run from the system clock */
    clock_periph_configure(CLOCKS_CLK_PERI_CTRL_AUXSRC_clk_sys);
    /* check clk_ref with logic analyzer */
    clock_gpout0_configure(MHZ(12), MHZ(12),
                           CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_clk_ref);
}

void cpu_init(void)
{
    /* initialize the Cortex-M core */
    cortexm_init();

    _cpu_reset();

    /* initialize stdio prior to periph_init() to allow use of DEBUG() there */
    stdio_init();

    /* trigger static peripheral initialization */
    periph_init();
}
