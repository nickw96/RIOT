/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_mfrc522
 *
 * @{
 * @file
 * @brief       Default configuration for MFRC522 RFID readers
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef MFRC522_PARAMS_H
#define MFRC522_PARAMS_H

#include "board.h"
#include "mfrc522.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Set default configuration parameters for the MFRC522 RFID readers
 * @{
 */
#ifndef MFRC522_PARAM_SPI
#define MFRC522_PARAM_SPI               (SPI_DEV(0))
#endif
#ifndef MFRC522_PARAM_SPI_CLK
#define MFRC522_PARAM_SPI_CLK           (SPI_CLK_10MHZ)
#endif
#ifndef MFRC522_PARAM_SPI_CS
#define MFRC522_PARAM_SPI_CS            (GPIO_PIN(PORT_F, 12))
#endif
#ifndef MFRC522_PARAM_IRQ_PIN
#define MFRC522_PARAM_IRQ_PIN           (GPIO_PIN(PORT_D, 14))
#endif
#ifndef MFRC522_PARAM_RST_PIN
#define MFRC522_PARAM_RST_PIN           (GPIO_PIN(PORT_D, 15))
#endif
#ifndef MFRC522_PARAMS
#define MFRC522_PARAMS                  { .spi = MFRC522_PARAM_SPI,  \
                                        .spi_clk = MFRC522_PARAM_SPI_CLK, \
                                        .spi_cs = MFRC522_PARAM_SPI_CS, \
                                        .irq_pin = MFRC522_PARAM_IRQ_PIN, \
                                        .rst_pin = MFRC522_PARAM_RST_PIN }
#endif
/**@}*/

/**
 * @brief   Configure MFRC522 devices
 */
static const mfrc522_params_t mfrc522_params[] =
{
    MFRC522_PARAMS
};

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_PARAMS_H */
/** @} */
