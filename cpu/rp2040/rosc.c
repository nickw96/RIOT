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
 * @brief       Implementation of the ring oscillator (ROSC)
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 * @author      Fabian Hüßler <fabian.huessler@ovgu.de>
 *
 * @}
 */

#include "vendor/RP2040.h"
#include "reg_atomic.h"

/**
 * @brief   Start the ring oscillator in default mode.
 *          The ROSC is running at boot time but may be turned off
 *          to save power when switching to the accurate XOSC.
 *          The default ROSC provides an instable frequency of 1.8 MHz to 12 MHz.
 */
void rosc_start(void)
{
    /* set drive strengths to default 0 */
    ROSC->FREQA.bit.PASSWD = 0;
    ROSC->FREQB.bit.PASSWD = 0;
    /* apply settings with magic value 0x9696 */
    ROSC->FREQA.bit.PASSWD = 0x9696U;
    ROSC->FREQB.bit.PASSWD = 0x9696U;

    /* default divider is 16 */
    ROSC->DIV.bit.DIV = 16U;
    reg_atomic_set(&ROSC->CTRL.reg, ROSC_CTRL_ENABLE_ENABLE << ROSC_CTRL_ENABLE_Pos);
    while (!ROSC->STATUS.bit.STABLE) { }
}

/**
 * @brief   Turn off the ROSC to save power.
 *          The system clock must be switched to to another lock source
 *          before the ROSC is stopped, other wise the chip will be lock up.
 */
void rosc_stop(void)
{
    reg_atomic_set(&ROSC->CTRL.reg, ROSC_CTRL_ENABLE_DISABLE << ROSC_CTRL_ENABLE_Pos);
}
