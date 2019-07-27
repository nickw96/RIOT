/*
 * Copyright 2019 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    drivers_mfrc522
 * @ingroup     drivers_rfid
 * @brief       Driver for the MFRC522 RFID reader
 *
 * This module contains a driver for the MFRC522 RFID reader connected via SPI.
 * The chip can alternatively be connected via UART and I2C, but this driver
 * does not support this.
 * @{
 *
 * @file
 * @brief       MFRC522 Device Driver
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef MFRC522_H
#define MFRC522_H

#include <stdbool.h>
#include <stdint.h>

#include "periph/gpio.h"
#include "periph/spi.h"
#include "event.h"
#include "xtimer.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MFRC522_EVENT_QUEUE) || defined(DOXYGEN)
/**
 * @brief   Event handler queue to enqueue MFRC522 events
 *
 * This is needed to access SPI, as mutually exclusive access to the SPI bus
 * from IRQ context is not going to work.
 */
#define MFRC522_EVENT_QUEUE EVENT_PRIO_LOWEST
#endif

#if !defined(MFRC522_BUF_SIZE) || defined(DOXYGEN)
/**
 * @brief   Maximum number of bytes to transfer from/to cards
 *
 * This value can be overwritten e.g. via `CFLAGS`.
 */
#define MFRC522_BUF_SIZE    128
#endif

/**
 * @brief   Connection parameters of the MFRC5222 driver
 */
typedef struct {
    spi_t spi;          /**< SPI bus to use for communication */
    spi_clk_t spi_clk;  /**< SPI clock speed to use */
    spi_cs_t spi_cs;    /**< GPIO connected to the chip select pin */
    gpio_t irq_pin;     /**< GPIO connected to the IRQ pin of the MFRC522 */
    gpio_t rst_pin;     /**< GPIO connected to the reset pin of the MFRC522 */
} mfrc522_params_t;

/**
 * @brief   Device descriptor of the MFRC522 driver
 */
typedef struct mfrc522 mfrc522_t;

/**
 * @brief   UID of a Proximity Integrated Circuit Card (PICC)
 */
typedef struct {
    uint8_t uid[10];    /**< The UID */
    uint8_t uid_len;    /**< The length of the UID: 4, 7 or 10 bytes */
} picc_uid_t;

/**
 * @brief   Signature of the callback run when an RFID chip is detected
 *
 * @param   dev     The device that detected the event
 * @param   uid     ID of the RFID tag that triggered the event
 * @param   data    Userdata given when the callback was set up
 */
typedef void (*mfrc522_cb_t)(mfrc522_t *dev, picc_uid_t *uid, void *user);

/**
 * @brief   Device descriptor of the MFRC522 driver
 */
struct mfrc522 {
    event_t ev;                     /**< Event object used for deferred ISR */
    mfrc522_params_t params;        /**< Connection parameters */
    mfrc522_cb_t callback;          /**< Function to call on events */
    void *userdata;                 /**< Data to pass to the callback */
    xtimer_t timer;                 /**< Soft timer used when polling for cards */
    mutex_t sync;                   /**< Used to block for async completion of transfer */
    uint8_t flags;                  /**< Internal flags for the event handler */
    uint8_t buf[MFRC522_BUF_SIZE];  /**< Buffer to hold data received from card (or data to send to card */
    uint8_t buf_fill;               /**< Number of bytes in buffer */
    uint8_t buf_pos;                /**< Index of the next byte to transfer to the FIFO during TX */
};

/**
 * @brief   Initialize the device descriptor and the hardware
 *
 * @param[out]  dev     Device descriptor to initialize
 * @param[in]   params  Connection parameters to use
 *
 * @retval  0           Success
 * @retval  -EINVAL     Called with invalid parameters
 * @retval  -EIO        Setting up GPIOs / acquiring SPI failed (invalid params?)
 * @retval  -ENODEV     Communication with the hardware failed (wiring?)
 *
 * This function will reset the reader to bring it into a known and operational
 * state. This also allows to verify that the configuration and wiring is
 * working.
 */
int mfrc522_init(mfrc522_t *dev, const mfrc522_params_t *params);

/**
 * @brief   Start scanning for cards and call the callback function when this
 *          happens.
 *
 * @param[out]  dev     Device descriptor of the MFRC522 to scan at
 * @param[in]   cb      Callback function to set
 * @param[in]   data    Value to pass to @p cb on events
 *
 * This function can be called multiple times without stopping the scan to
 * change the callback function or its argument.
 *
 * The callback function will be called from thread context from the event
 * handler thread corresponding to @ref MFRC522_EVENT_QUEUE.
 * @warning Avoid calling functions that can block for more than very brief
 *          periods of time, unless you provide a distinct event handler thread
 *          for this driver: By default the shared event handler thread is used.
 *          Blocking in that context of the shared event handler can degrade
 *          the real time properties of the whole system.
 */
void mfrc522_scan_start(mfrc522_t *dev, mfrc522_cb_t cb, void *data);

/**
 * @brief   Stop scanning for new cards.
 *
 * @param[out]  dev     Device descriptor of the MFRC522 to stop scanning
 *                      with.
 */
void mfrc522_scan_stop(mfrc522_t *dev);


#ifdef __cplusplus
}
#endif

#endif /* MFRC522_H */
/** @} */
