/*
 * Copyright 2019 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_pms5003
 *
 * @{
 *
 * @file
 * @brief       Implementation of the PMS5003 Particulate Matter Sensor
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <errno.h>
#include <string.h>

#include "assert.h"
#include "irq.h"
#include "mutex.h"
#include "pms5003.h"
#include "pms5003_constants.h"
#include "pms5003_params.h"

#ifdef MODULE_OD
/* if module od is used and debugging enabled, received messages are dumped */
#include "od.h"
#endif /* USEMODULE_OD */

#define ENABLE_DEBUG        (0)
#include "debug.h"

#define PMS5003_NUM     (sizeof(pms5003_params) / sizeof(pms5003_params[0]))

#ifdef __cplusplus
extern "C" {
#endif

extern pms5003_dev_t pms5003_devs[PMS5003_NUM];

static inline pms5003_dev_t * _get_dev(pms5003_t id)
{
    if (id >= PMS5003_NUM) {
        return NULL;
    }

    return &pms5003_devs[id];
}

static void _error_callbacks(pms5003_dev_t *dev, pms5003_error_t error)
{
    for (pms5003_callbacks_t *i = dev->cbs; i != NULL; i = i->next) {
        if (i->cb_error) {
            i->cb_error(error, i->userdata);
        }
    }
}

static void _handle_received(pms5003_dev_t *dev)
{
#if defined(MODULE_OD) && ENABLE_DEBUG
    od_hex_dump(dev->buf, sizeof(dev->buf), OD_WIDTH_DEFAULT);
#endif /* defined(MODULE_OD) && ENABLE_DEBUG */

    if (!dev->cbs) {
        DEBUG("[pms5003] No callbacks, skip parsing received data\n");
        return;
    }

    uint16_t length = (dev->buf[0] << 8) | dev->buf[1];

    if (sizeof(dev->buf) - sizeof(length) != length) {
        DEBUG("[pms5003] Message invalid: Incorrect length: %u\n",
              (unsigned)length);
        _error_callbacks(dev, PMS5003_ERROR_FORMAT);
        return;
    }

    uint16_t checksum_got = (dev->buf[28] << 8) | dev->buf[29];
    uint16_t checksum_exp = PMS5003_START_SYMBOL1 + PMS5003_START_SYMBOL2;
    for (unsigned i = 0; i < sizeof(dev->buf) - sizeof(checksum_got); i++) {
        checksum_exp += dev->buf[i];
    }

    if (checksum_got != checksum_exp) {
        DEBUG("[pms5003] Checksum error: Expected %x, got %x\n",
              (unsigned)checksum_exp, (unsigned)checksum_got);
        _error_callbacks(dev, PMS5003_ERROR_FORMAT);
        return;
    }

    pms5003_data_t data = {
        .pm_1_0     = (dev->buf[ 2] << 8) | dev->buf[ 3],
        .pm_2_5     = (dev->buf[ 4] << 8) | dev->buf[ 5],
        .pm_10_0    = (dev->buf[ 6] << 8) | dev->buf[ 7],
        .pm_a_1_0   = (dev->buf[ 8] << 8) | dev->buf[ 9],
        .pm_a_2_5   = (dev->buf[10] << 8) | dev->buf[11],
        .pm_a_10_0  = (dev->buf[12] << 8) | dev->buf[13],
        .n_0_3      = (dev->buf[14] << 8) | dev->buf[15],
        .n_0_5      = (dev->buf[16] << 8) | dev->buf[17],
        .n_1_0      = (dev->buf[18] << 8) | dev->buf[19],
        .n_2_5      = (dev->buf[20] << 8) | dev->buf[21],
        .n_5_0      = (dev->buf[22] << 8) | dev->buf[23],
        .n_10_0     = (dev->buf[24] << 8) | dev->buf[25],
    };

    for (pms5003_callbacks_t *i = dev->cbs; i != NULL; i = i->next) {
        if (i->cb_data) {
            i->cb_data(&data, i->userdata);
        }
    }
}

static void _uart_cb(void *_dev, uint8_t data)
{
    pms5003_dev_t *dev = _dev;
    switch (dev->state) {
        default:
        case PMS5003_STATE_WAITING_FOR_START:
            if (data == PMS5003_START_SYMBOL1) {
                /* Received first half of start symbol */
                dev->state = PMS5003_STATE_START_COMPLETING;
                return;
            }
            if (++dev->pos > PMS5003_START_TIMEOUT) {
                DEBUG("[pms5003] Waiting for start symbol timed out\n");
                dev->pos = 0;
                _error_callbacks(dev, PMS5003_ERROR_TIMEOUT);
            }
            break;
        case PMS5003_STATE_START_COMPLETING:
            if (data == PMS5003_START_SYMBOL2) {
                /* Start symbol completely received */
                dev->state = PMS5003_STATE_RECEIVING;
                dev->pos = 0;
                return;
            }
            DEBUG("[pms5003] Received incomplete start symbol --> wait again\n");
            dev->pos++;
            dev->state = PMS5003_STATE_WAITING_FOR_START;
            break;
        case PMS5003_STATE_RECEIVING:
            dev->buf[dev->pos++] = data;
            if (dev->pos >= sizeof(dev->buf)) {
                dev->state = PMS5003_STATE_PROCESSING;
                DEBUG("[pms5003] Received all data\n");
                _handle_received(dev);
                dev->pos = 0;
                dev->state = PMS5003_STATE_WAITING_FOR_START;
            }
            break;
        case PMS5003_STATE_PROCESSING:
            /* Ignoring data while processing old message */
            break;
    }
}

int pms5003_init(pms5003_dev_t *dev, const pms5003_params_t *params)
{
    if (!dev || !params) {
        return -EINVAL;
    }

    memset(dev, 0x00, sizeof(pms5003_dev_t));
    dev->params = *params;

    if (dev->params.set != GPIO_UNDEF) {
        if (gpio_init(dev->params.set, GPIO_OUT)) {
            return -EIO;
        }

        gpio_set(dev->params.set);
    }

    if (dev->params.reset != GPIO_UNDEF) {
        if (gpio_init(dev->params.reset, GPIO_OUT)) {
            return -EIO;
        }

        gpio_set(dev->params.reset);
    }

    if (uart_init(dev->params.uart, PMS5003_BAUD, _uart_cb, dev) != UART_OK) {
        return -EIO;
    }

    return 0;
}

void pms5003_add_callbacks(pms5003_t id,
                           pms5003_callbacks_t *callbacks)
{
    pms5003_dev_t *dev = _get_dev(id);
    assert(dev && callbacks);

    if (!dev || !callbacks) {
        return;
    }

    /* replace callbacks and data atomically to prevent mischief */
    unsigned state = irq_disable();
    callbacks->next = dev->cbs;
    dev->cbs = callbacks;
    irq_restore(state);

    DEBUG("[pms5003] Added callbacks: data = %p, error = %p, userdata = %p\n",
          (void *)callbacks->cb_data, (void *)callbacks->cb_error,
          callbacks->userdata);
}

void pms5003_del_callbacks(pms5003_t id,
                           pms5003_callbacks_t *callbacks)
{
    pms5003_dev_t *dev = _get_dev(id);
    assert(dev && callbacks);

    if (!dev || !callbacks) {
        return;
    }

    /* replace callbacks and data atomically to prevent mischief */
    unsigned state = irq_disable();

    /* A double linked list would be O(1) instead of O(n), but for the average
     * use case with few (often only 1 entry) in the list, a single linked
     * list is better
     */
    pms5003_callbacks_t **list = &dev->cbs;
    while (*list) {
        if (*list == callbacks) {
            *list = callbacks->next;
            irq_restore(state);
            return;
        }
    }
    irq_restore(state);

    DEBUG("[pms5003] Failed to remove callbacks: data = %p, error = %p, "
          "userdata = %p\n",
          (void *)callbacks->cb_data, (void *)callbacks->cb_error,
          callbacks->userdata);
}

/**
 * @brief   Structure holding the data for the pms5003_read function
 */
typedef struct {
    pms5003_data_t *dest;
    pms5003_error_t error;
    mutex_t mutex;
} pms5003_read_data_t;

static void _read_cb_data(const pms5003_data_t *data, void *userdata)
{
    pms5003_read_data_t *rdata = userdata;

    *rdata->dest = *data;
    mutex_unlock(&rdata->mutex);
}

static void _read_cb_error(pms5003_error_t error, void *userdata)
{
    pms5003_read_data_t *rdata = userdata;

    rdata->error = error;
    mutex_unlock(&rdata->mutex);
}

int pms5003_read(pms5003_t id, pms5003_data_t *data)
{
    pms5003_dev_t *dev = _get_dev(id);

    if (!dev) {
        return -ENODEV;
    }

    if (!data) {
        return -EINVAL;
    }

    pms5003_read_data_t rdata = {
        .mutex = MUTEX_INIT_LOCKED,
        .dest = data,
        .error = PMS5003_NO_ERROR,
    };

    pms5003_callbacks_t callbacks = {
        .cb_data = _read_cb_data,
        .cb_error = _read_cb_error,
        .userdata = &rdata
    };

    pms5003_add_callbacks(id, &callbacks);

    /* Lock already locked mutex --> blocks until unlock from callback */
    DEBUG("[pms5003] pms5003_read() blocks until data is received\n");
    mutex_lock(&rdata.mutex);

    pms5003_del_callbacks(id, &callbacks);

    if (rdata.error == PMS5003_NO_ERROR) {
        DEBUG("[pms5003] pms5003_read() succeeded\n");
        return 0;
    }
    DEBUG("[pms5003] pms5003_read() failed with error %d\n", rdata.error);
    return -EIO;
}
