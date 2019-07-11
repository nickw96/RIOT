/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_pms5003
 *
 * @{
 * @file
 * @brief       Default configuration for the PMS5003 particulate matter sensor
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef PMS5003_PARAMS_H
#define PMS5003_PARAMS_H

#include "board.h"
#include "pms5003.h"
#include "saul_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Set default configuration parameters for the PMS5003
 * @{
 */
#ifndef PMS5003_PARAM_UART
#define PMS5003_PARAM_UART            (UART_DEV(2))
#endif
#ifndef PMS5003_PARAM_SET
#define PMS5003_PARAM_SET             (GPIO_UNDEF)
#endif
#ifndef PMS5003_PARAM_RESET
#define PMS5003_PARAM_RESET           (GPIO_UNDEF)
#endif
#ifndef PMS5003_PARAMS
#define PMS5003_PARAMS                { .uart = PMS5003_PARAM_UART,  \
                                        .set = PMS5003_PARAM_SET, \
                                        .reset = PMS5003_PARAM_RESET }
#endif
/**@}*/

/**
 * @name    Set default SAUL info text for the PMS5003
 * @{
 */
#ifndef PMS5003_SAULINFO
#define PMS5003_SAULINFO              { .name = "PMS5003 PM1.0, PM2.5, PM10" }
#endif

/**@}*/

/**
 * @brief   Configure PMS5003 devices
 */
static const pms5003_params_t pms5003_params[] =
{
    PMS5003_PARAMS
};

/**
 * @brief   Allocate and configure entries to the SAUL registry
 */
static const saul_reg_info_t pms5003_saul_info[] =
{
    PMS5003_SAULINFO
};

#ifdef __cplusplus
}
#endif

#endif /* PMS5003_PARAMS_H */
/** @} */
