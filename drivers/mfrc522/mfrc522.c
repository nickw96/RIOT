/*
 * Copyright 2019 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_mfrc522.c
 * @brief       Driver for the MFRC522 RFID reader
 * @{
 *
 * @file
 * @brief       MFRC522 RFID Reader Device Driver
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "atomic_utils.h"
#include "event.h"
#include "event/thread.h"
#include "kernel_defines.h"
#include "mfrc522.h"
#include "mfrc522_constants.h"
#include "periph/gpio.h"
#include "xtimer.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/**
 * @brief   Acquire exclusive access to the SPI bus of the given MFRC522 device
 *          and configure the device according to the params
 *
 * @param[in]   dev     Device descriptor of the MFRC522
 *
 * @retval  0           Success
 * @retval  -EIO        Failure
 */
static inline int acquire(mfrc522_t *dev)
{
    if (SPI_OK != spi_acquire(dev->params.spi, dev->params.spi_cs,
                              MFRC522_SPI_MODE, dev->params.spi_clk)) {
        return -EIO;
    }

    return 0;
}

/**
 * @brief   Release exclusive access of the SPI bus
 *
 * @param[in]   dev     Device descriptor of the MFRC522
 */
static inline void release(mfrc522_t *dev)
{
    spi_release(dev->params.spi);
}

/**
 * @brief   Reads the register specified by address @p addr
 *
 * @param   addr    Address of the register to read
 * @return  The contents of the register address @p addr
 */
static uint8_t reg_read(mfrc522_t *dev, uint8_t addr)
{
    /* The access scheme of the MFRC522 differs from standard register access,
     * therefore we cannot use spi_read_regs() */
    uint8_t buf[2] = {
        MFRC522_READ | (addr << 1),
        0x00
    };

    /* Use one SPI transfer, so whole access can be done in one DMA transfer,
     * if DMA is supported and used by the target */
    spi_transfer_bytes(dev->params.spi, dev->params.spi_cs, false, buf, buf, 2);
    return buf[1];
}

/**
 * @brief   Writes the given byte to the register specified by address @p addr
 *
 * @param   addr    Address of the register to read
 * @param   data    Data byte to write
 */
static void reg_write(mfrc522_t *dev, uint8_t addr, uint8_t data)
{
    /* The access scheme of the MFRC522 differs from standard register access,
     * therefore we cannot use spi_read_regs() */
    uint8_t buf[2] = {
        MFRC522_WRITE | (addr << 1),
        data
    };

    /* Use one SPI transfer, so whole access can be done in one DMA transfer,
     * if DMA is supported and used by the target */
    spi_transfer_bytes(dev->params.spi, dev->params.spi_cs, false,
                       buf, NULL, 2);
}

/**
 * @brief   Enable/disable output energy carrier
 *
 * @param[in]   dev     Device to enable energy carrier on
 * @param[in]   on      `true` for enabling energy carrier
 */
static void mfrc522_set_power(mfrc522_t *dev, bool on)
{
    const uint8_t tx_on = MFRC522_REG_TX_CONTROL_TX2_ON
                          | MFRC522_REG_TX_CONTROL_TX1_ON;

    uint8_t val = reg_read(dev, MFRC522_REG_TX_CONTROL);

    if (on) {
        val |= tx_on;
    }
    else {
        val &= ~tx_on;
    }

    reg_write(dev, MFRC522_REG_TX_CONTROL, val);
}

/**
 * @brief   Send a command to the MFRC522
 *
 * @param[in]   dev     Device descriptor of the MFRC522
 * @param[in]   cmd     Command to send
 */
static void cmd(mfrc522_t *dev, uint8_t cmd)
{
    reg_write(dev, MFRC522_REG_CMD, cmd & 0x0f);
}

/**
 * @brief   Print the current MFRC522 status and IRQ register contents if
 *          `ENABLE_DEBUG` is set
 */
static void debug_print_status(mfrc522_t *dev)
{
    DEBUG("[mfrc522] IRQ_COM: 0x%02x, IRQ_DIV: 0x%02x, STATUS1: 0x%02x, "
          "STATUS2: 0x%02x\n",
          reg_read(dev, MFRC522_REG_IRQ_COM),
          reg_read(dev, MFRC522_REG_IRQ_DIV),
          reg_read(dev, MFRC522_REG_STATUS_1),
          reg_read(dev, MFRC522_REG_STATUS_2));
}

/**
 * @brief   Print the current MFRC522 error codes if `ENABLE_DEBUG` is set
 */
static void debug_print_status(mfrc522_t *dev)
{
    if (IS_ACTIVE(ENABLE_DEBUG)) {
        uint8_t err = reg_read(dev, MFRC522_REG_ERROR_MASK);
        DEBUG("[mfrc522] Errors: \n");
        if (err & MFRC522_REG_ERROR_UNEXPECTED_WRITE) {
            DEBUG("    - Unexpected write to FIFO\n");
        }
        if (err & MFRC522_REG_ERROR_OVERHEATED) {
            DEBUG("    - Thermal shutdown of antenna drivers\n");
        }
        if (err & MFRC522_REG_ERROR_OVERFLOW) {
            DEBUG("    - FIFO overflown\n");
        }
        if (err & MFRC522_REG_ERROR_COLLISION) {
            DEBUG("    - Bit collision occured\n");
        }
        if (err & MFRC522_REG_ERROR_CRC) {
            DEBUG("    - CRC mismatch\n");
        }
        if (err & MFRC522_REG_ERROR_PARITY) {
            DEBUG("    - Parity mismatch\n");
        }
        if (err & MFRC522_REG_ERROR_PROTOCOL) {
            DEBUG("    - Protocol error\n");
        }
    }
}

/**
 * @brief   Transfer data in `dev->buf` to card and store the reply there
 *
 * @param[in,out]   dev         Device descriptor of the MFRC522
 *
 * @return                      Number of bytes received
 * @retval  -EOVERFLOW          Response is larger than @p buf_size
 * @retval  -EBUSY              Another transfer is already in progress
 * @retval  -EIO                Transfer failed
 */
static ssize_t card_transfer(mfrc522_t *dev)
{
    assert(dev->buf_fill <= MFRC522_BUF_SIZE);
    acquire(dev);
    if (atomic_load_u8(&dev->flags) & MFRC522_FLAG_BUSY) {
        release(dev);
        return -EBUSY;
    }
    uint8_t irq_mask = MFRC522_IRQ_EN_COM_INVERT /* waiting on falling edge */
                       | MFRC522_IRQ_EN_COM_TX /* wait for TX completion */
                       | MFRC522_IRQ_EN_COM_IDLE /* wait for completion of command */
                       | MFRC522_IRQ_EN_COM_ERROR;

    uint8_t len = dev->buf_fill;
    if (len > MFRC522_FIFO_SIZE) {
        /* if data to send is larger than the FIFO, we need to refill the
         * FIFO during transmission. The FIFO low IRQ will wake us up before
         * the FIFO is fully drained. */
        irq_mask |= MFRC522_IRQ_EN_COM_FIFO_LOW;
        len = MFRC522_FIFO_SIZE;
    }

    cmd(dev, MFRC522_CMD_IDLE);
    for (uint8_t i = 0; i < len; i++) {
        reg_write(dev, MFRC522_REG_FIFO, dev->buf[i]);
    }
    dev->buf_pos = len;
    /* make sure mutex is locked now, so that we indeed block later on */
    mutex_trylock(&dev->sync);
    reg_write(dev, MFRC522_REG_IRQ_EN_COM, irq_mask);
    cmd(dev, MFRC522_CMD_TX_RX);
    reg_write(dev, MFRC522_REG_BIT_FRAMING, MFRC522_REG_BIT_FRAMING_START | MFRC522_REG_BIT_FRAMING_BITS_7);
    atomic_fetch_or_u8(&dev->flags, MFRC522_FLAG_BUSY);
    release(dev);
    /* wait for IRQ handler to signal completion */
    mutex_lock(&dev->sync);
    uint8_t flags = atomic_load_u8(&dev->flags);
    atomic_fetch_and_u8(&dev->flags, ~(MFRC522_FLAG_ERROR | ~MFRC522_FLAG_OVERFLOW));
    if (flags & MFRC522_FLAG_ERROR) {
        return -EIO;
    }
    if (flags & MFRC522_FLAG_OVERFLOW) {
        return -EIO;
    }

    return dev->buf_fill;
}

/**
 * @brief   Deferred IRQ handler
 *
 * @param[in]   ev      Event structure of the MFRC522 descriptor for which
 *                      work is to do
 *
 * As mutually exclusive access to a shared SPI bus is not possible from IRQ
 * context, the IRQ handler has to be deferred to thread context. But this
 * also offers advantages regarding the real time behavior, as (at least by
 * default) nested interrupts are not used by RIOT.
 */
static void irq_ev_handler(event_t *ev)
{
    mfrc522_t *dev = container_of(ev, mfrc522_t, ev);
    acquire(dev);
    uint8_t flags = atomic_load_u8(&dev->flags);
    if (flags & MFRC522_FLAG_EXTIRQ) {
        DEBUG("[mfrc522] IRQ\n");
        debug_print_status(dev);

        uint8_t irq_flags = reg_read(dev, MFRC522_REG_IRQ_COM);
        if (irq_flags & MFRC522_IRQ_COM_ERROR) {
            debug_print_error(dev);
            atomic_fetch_or_u8(&dev->flags, MFRC522_FLAG_ERROR);
            mutex_unlock(&dev->sync);
        } else if (irq_flags & MFRC522_IRQ_COM_TX) {
            DEBUG("[mfrc522] TX competed\n");
            /* enable IRQs relevant for RX now */
            req_write(dev, MFRC522_REG_IRQ_EN_COM,
                      MFRC522_IRQ_EN_COM_INVERT
                      | MFRC522_IRQ_EN_COM_RX
                      | MFRC522_IRQ_EN_COM_IDLE
                      | MFRC522_IRQ_EN_COM_FIFO_HIGH);
        }
        else if (irq_flags & MFRC522_IRQ_COM_FIFO_LOW) {
            uint8_t to_write = dev->buf_fill - dev->buf_pos;
            uint8_t capacity = reg_read(dev, MFRC522_REG_FIFO_FILL);
            if (to_write > capacity) {
                to_write = capacity;
            }
            for (uint8_t i = 0; i < to_write; i++) {
                reg_write(dev, MFRC522_REG_FIFO, dev->buf[dev->pos++]);
            }
        }
        else if (irq_flags & (MFRC522_IRQ_COM_FIFO_HIGH | MFRC522_IRQ_COM_RX | MFRC522_IRQ_COM_IDLE)) {
            uint8_t to_read = reg_read(dev, MFRC522_REG_FIFO_FILL);
            if (to_read + dev->buf_fill > MFRC522_BUF_SIZE) {
                cmd(dev, MFRC522_CMD_IDLE);
                atomic_fetch_or_u8(&dev->flags, MFRC522_FLAG_OVERFLOW);
                DEBUG("[mfrc522] RX overflown\n");
                mutex_unlock(&dev->sync);
            }
            else {
                while (to_read--) {
                    dev->buf[dev->buf_fill++] = reg_read(dev, MFRC522_REG_FIFO);
                }
                if (irq_flags & (MFRC522_IRQ_COM_RX | MFRC522_IRQ_COM_IDLE)) {
                    DEBUG("[mfrc522] RX completed\n");
                    mutex_unlock(&dev->sync);
                }
            }
        }
        atomic_fetch_and_u8(&dev->flags, ~MFRC522_FLAG_EXTIRQ);
        reg_write(dev, MFRC522_REG_IRQ_COM, MFRC522_IRQ_COM_CLEAR);
    }
    else if (!(flags & MFRC522_FLAG_BUSY)) {
        DEBUG("[mfrc522] polling\n");
        reg_write(dev, MFRC522_REG_FIFO, PICC_CMD_REQA);
        cmd(dev, MFRC522_CMD_TX_RX);
        reg_write(dev, MFRC522_REG_BIT_FRAMING, MFRC522_REG_BIT_FRAMING_START | MFRC522_REG_BIT_FRAMING_BITS_7);
        if (flags & MFRC522_FLAG_POLLING) {
            xtimer_set(&dev->timer, MFRC522_POLLING_TIMEOUT_MS * US_PER_MS);
        }
    }
    release(dev);
}

static void irq_handler(void *_dev)
{
    mfrc522_t *dev = _dev;
    dev->flags |= MFRC522_FLAG_EXTIRQ;
    event_post(MFRC522_EVENT_QUEUE, &dev->ev);
}

static void timeout_handler(void *_dev)
{
    mfrc522_t *dev = _dev;
    event_post(MFRC522_EVENT_QUEUE, &dev->ev);
}

static int connectivity_check(mfrc522_t *dev)
{
    uint8_t version = reg_read(dev, MFRC522_REG_VERSION);
    switch (version) {
        case MFRC522_VERSION_1:
            DEBUG("[mfrc522] MFRC522 Version 1.0 detected\n");
            break;
        case MFRC522_VERSION_2:
            DEBUG("[mfrc522] MFRC522 Version 2.0 detected\n");
            break;
        default:
            DEBUG("[mfrc522] Unknown device connected, version = 0x%02x\n",
                  version);
            return -1;
    }

    return 0;
}

int mfrc522_init(mfrc522_t *dev, const mfrc522_params_t *params)
{
    if (!dev || !params) {
        return -EINVAL;
    }

    memset(dev, 0, sizeof(mfrc522_t));
    dev->params = *params;
    dev->ev.handler = irq_ev_handler;
    dev->timer.callback = timeout_handler;
    dev->timer.arg = dev;
    const mutex_t mut_locked = MUTEX_INIT_LOCKED;
    dev->sync = mut_locked;

    if (SPI_OK != spi_init_cs(dev->params.spi, dev->params.spi_cs)) {
        DEBUG("[mfrc522] Initializing CS failed\n");
        return -EIO;
    }

    if (acquire(dev)) {
        DEBUG("[mfrc522] Acquiring SPI failed\n");
        return -EIO;
    }

    if (dev->params.rst_pin != GPIO_UNDEF) {
        /* Use hard reset via reset pin */
        if (gpio_init(dev->params.rst_pin, GPIO_OUT)) {
            release(dev);
            DEBUG("[mfrc522] Failed to init RST pin\n");
            return -EIO;
        }
        gpio_clear(dev->params.rst_pin);
        xtimer_usleep(MFRC522_RESET_TIME_US);
        gpio_set(dev->params.rst_pin);
    }
    else {
        /* Fall back to soft reset, if reset pin is not connected */
        cmd(dev, MFRC522_CMD_RST);
    }
    xtimer_usleep(MFRC522_STARTUP_TIME_MS * US_PER_MS);

    if (connectivity_check(dev)) {
        release(dev);
        return -ENODEV;
    }

    reg_write(dev, MFRC522_REG_IRQ_EN_DIV, MFRC522_IRQ_EN_DIV_MFIN);
    /* The 12 prescale value is stored in two registers: The 4 most significant
     * bits are stored in MFRC522_REG_TIMER_MODE, the remaining 8 bits in
     * MFRC522_REG_TIMER_PRESCALER. This sets the timer to
     * 14.56 MHz / (2 * 0xd3e + 1) = 2 kHz. */
    const uint16_t timer_prescale = 0xd3e;
    reg_write(dev, MFRC522_REG_TIMER_MODE,
              MFRC522_REG_TIMER_MODE_AUTO | (timer_prescale >> 8));
    reg_write(dev, MFRC522_REG_TIMER_PRESCALER, (uint8_t)timer_prescale);
    const uint16_t timer_reload = 0x001e;
    reg_write(dev, MFRC522_REG_TIMER_RELOAD_MSB, timer_reload >> 8);
    reg_write(dev, MFRC522_REG_TIMER_RELOAD_LSB, (uint8_t)timer_reload);
    reg_write(dev, MFRC522_REG_FORCE_ASK, MFRC522_REG_FORCE_ASK_ENABLED);
    reg_write(dev, MFRC522_REG_MODE, MFRC522_REG_MODE_RFU
                                     | MFRC522_REG_MODE_TX_WAIT_RF
                                     | MFRC522_REG_MODE_MFIN_ACTIVE_HIGH
                                     | MFRC522_REG_MODE_CRC_PRESET_6363);

    release(dev);

    if (gpio_init_int(dev->params.irq_pin, GPIO_IN_PU, GPIO_FALLING,
                      irq_handler, dev)) {
        return -EIO;
    }

    return 0;
}

void mfrc522_scan_start(mfrc522_t *dev, mfrc522_cb_t cb, void *data)
{
    acquire(dev);
    dev->callback = cb;
    dev->userdata = data;
    if (!(dev->flags & MFRC522_FLAG_POLLING)) {
        dev->flags |= MFRC522_FLAG_POLLING;
        event_post(MFRC522_EVENT_QUEUE, &dev->ev);
        mfrc522_set_power(dev, true);
        reg_write(dev, MFRC522_REG_IRQ_EN_COM,
                  MFRC522_IRQ_EN_COM_INVERT | MFRC522_IRQ_EN_COM_RX);
    }
    release(dev);
}

void mfrc522_scan_stop(mfrc522_t *dev)
{
    acquire(dev);
    if (dev->flags & MFRC522_FLAG_POLLING) {
        dev->flags &= ~MFRC522_FLAG_POLLING;
        xtimer_remove(&dev->timer);
        mfrc522_set_power(dev, false);
    }
    release(dev);
}
