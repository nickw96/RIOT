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
 * @brief       Constants and magic numbers used in the PMS5003 driver
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef PMS5003_CONSTANTS_H
#define PMS5003_CONSTANTS_H

#include "board.h"
#include "pms5003.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Symbol rate of the PMS5003 UART interface
 */
#define PMS5003_BAUD            (9600)

/**
 * @brief   First half of the start symbol marking the beginning of a message
 */
#define PMS5003_START_SYMBOL1   (0x42)

/**
 * @brief   Second half of the start symbol marking the beginning of a message
 */
#define PMS5003_START_SYMBOL2   (0x4d)

/**
 * @brief   Number of symbols on UART to wait for the start symbol before
 *          timing out
 *
 * One complete message (start symbol + data + checksum) is 32B. Waiting for
 * two messages (= 64 symbols) seems to be reasonable
 */
#define PMS5003_START_TIMEOUT   (64)
#ifdef __cplusplus
}
#endif

#endif /* PMS5003_CONSTANTS_H */
/** @} */
