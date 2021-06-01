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
 * @brief       Implementation of the crystal oscillator (XOSC)
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 * @author      Fabian Hüßler <fabian.huessler@ovgu.de>
 *
 * @}
 */

#include <assert.h>

#include "macros/units.h"
#include "vendor/RP2040.h"
#include "reg_atomic.h"

static inline uint32_t _xosc_conf_sartup_delay(uint32_t f_crystal_mhz, uint32_t t_stable_ms)
{
    return (((f_crystal_mhz / 1000) * t_stable_ms) + 128) / 256;
}

void xosc_start(uint32_t f_ref)
{
    assert(f_ref == MHZ(12));
    uint32_t delay = _xosc_conf_sartup_delay(f_ref, 1);
    XOSC->STARTUP.bit.DELAY = delay;
    XOSC->CTRL.bit.ENABLE = XOSC_CTRL_ENABLE_ENABLE;
    while (!XOSC->STATUS.bit.STABLE) { }
}

void xosc_stop(void)
{
    XOSC->CTRL.bit.ENABLE = XOSC_CTRL_ENABLE_DISABLE;
}
