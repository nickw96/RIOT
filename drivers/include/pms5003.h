/*
 * Copyright 2019 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    drivers_pms5003 PMS5003 Particulate Matter Sensor
 * @ingroup     drivers_sensors
 * @brief       Driver for the Plantower PMS5003 Particulate Matter Sensor
 *
 * @{
 *
 * @file
 * @brief       PMS5003 Particulate Matter Sensor
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef PMS5003_H
#define PMS5003_H

#include <stdint.h>

#include "periph/gpio.h"
#include "periph/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Enum over the PMS5003 driver states
 */
typedef enum {
    PMS5003_STATE_WAITING_FOR_START,    /**< Waiting for start symbol (`0x42`) */
    PMS5003_STATE_START_COMPLETING,     /**< Waiting for end of start symbol (`0x4d`) */
    PMS5003_STATE_RECEIVING,            /**< Receiving data */
    PMS5003_STATE_PROCESSING,           /**< Processing received data */
} pms5003_state_t;

/**
 * @brief   Enum over all PMS5003 errors
 */
typedef enum {
    PMS5003_NO_ERROR,               /**< No error occurred */
    PMS5003_ERROR_CHECKSUM,         /**< Checksum mismatch */
    PMS5003_ERROR_FORMAT,           /**< Message format error */
    PMS5003_ERROR_TIMEOUT,          /**< Waiting for start symbol timed out */
    PMS5003_ERROR_NUMOF,            /**< Number of PMS5003 errors */
} pms5003_error_t;

/**
 * @brief   Structure holding all measurement data of the PMS5003
 *
 * All concentrations are given in µg/m³.
 */
typedef struct {
    uint16_t pm_1_0;    /**< PM1.0 concentration */
    uint16_t pm_2_5;    /**< PM2.0 concentration */
    uint16_t pm_10_0;   /**< PM10.0 concentration */
    uint16_t pm_a_1_0;  /**< PM1.0 concentration under atmospheric environment */
    uint16_t pm_a_2_5;  /**< PM2.5 concentration under atmospheric environment */
    uint16_t pm_a_10_0; /**< PM10 concentration under atmospheric environment */
    uint16_t n_0_3;     /**< Number of particles > 0.3µm in 0.1l air */
    uint16_t n_0_5;     /**< Number of particles > 0.5µm in 0.1l air */
    uint16_t n_1_0;     /**< Number of particles > 1.0µm in 0.1l air */
    uint16_t n_2_5;     /**< Number of particles > 2.5µm in 0.1l air */
    uint16_t n_5_0;     /**< Number of particles > 5.0µm in 0.1l air */
    uint16_t n_10_0;    /**< Number of particles > 10.0µm in 0.1l air */
} pms5003_data_t;

/**
 * @brief   Device descriptor of the PMS5003 sensor
 * @see     pms5003_dev
 */
typedef struct pms5003_dev pms5003_dev_t;

typedef uint8_t pms5003_t;

/**
 * @brief   Signature of the function called when the PMS5003 sensor
 *          received a measurement
 * @param   dev         PMS5003 device that triggered the callback
 * @param   data        The data received from the sensor
 * @param   userdata    User data supplied in @ref pms5003_set_callbacks
 */
typedef void (*pms5003_cb_data_t)(const pms5003_data_t *data, void *userdata);

/**
 * @brief   Signature of the function called when receiving data of the
 *          PMS5003 sensor failed
 * @param   dev         PMS5003 device that triggered the callback
 * @param   error       Error code indicating what went wrong
 * @param   userdata    User data supplied in @ref pms5003_set_callbacks
 */
typedef void (*pms5003_cb_error_t)(pms5003_error_t error, void *userdata);

/**
 * @brief   Callbacks for an PMS5003 particulate matter sensor
 */
typedef struct pms5003_callbacks {
    struct pms5003_callbacks *next; /**< Next registered callbacks */
    pms5003_cb_data_t cb_data;      /**< Called when data was received */
    pms5003_cb_error_t cb_error;    /**< Called when an error occured */
    void *userdata;                 /**< Data to pass to the callbacks */
} pms5003_callbacks_t;

/**
 * @brief   Structure holding the I/O parameters of the PMS5003 sensor
 */
typedef struct {
    uart_t uart;        /**< UART interface the sensor is connected to */
    gpio_t set;         /**< GPIO connected to the SET pin, or `GPIO_UNDEF` */
    gpio_t reset;       /**< GPIO connected to the RESET pin, or `GPIO_UNDEF` */
} pms5003_params_t;

/**
 * @brief   Device descriptor of the PMS5003 sensor
 */
struct pms5003_dev {
    pms5003_params_t params;    /**< Parameter of the PMS5003 driver */
    pms5003_callbacks_t *cbs;   /**< Registered callbacks */
    uint8_t buf[30];            /**< Buffer holding the received data */
    uint8_t pos;                /**< Position in the buffer while receiving */
    pms5003_state_t state;      /**< Current driver state */
};

/**
 * @brief           Initialize the PMS5003 driver
 *
 * @param   dev     Device to initialize
 * @param   params  Information on how the PMS5003 is connected
 *
 * @retval  0       Success
 * @retval  -EINVAL Called with invalid argument(s)
 * @retval  -EIO    IO failure
 */
int pms5003_init(pms5003_dev_t *dev, const pms5003_params_t *params);

/**
 * @brief           Register the given callbacks
 * @param dev       The PMS5003 device to add the callback functions to
 * @param callbacks Functions to call when a measurement completed / an error
 *                  occurred
 */
void pms5003_add_callbacks(pms5003_t dev, pms5003_callbacks_t *callbacks);

/**
 * @brief           Unregister the given callbacks
 * @param dev       The PMS5003 device to remove the given callbacks from
 * @param callbacks Callbacks to unregister
 */
void pms5003_del_callbacks(pms5003_t dev, pms5003_callbacks_t *callbacks);

/**
 * @brief           Perform a single read from a PMS5003 sensor in blocking mode
 * @param   dev     PMS5003 sensor to read data from
 * @param   data    The result will be stored here
 *
 * @retval  0       Success
 * @retval  -EINVAL @p data is `NULL`
 * @retval  -ENODEV @p dev is invalid
 * @retval  -EIO    Reading data failed
 */
int pms5003_read(pms5003_t dev, pms5003_data_t *data);
#ifdef __cplusplus
}
#endif

#endif /* PMS5003_H */
/** @} */
