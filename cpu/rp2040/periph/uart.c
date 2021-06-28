/*
 * Copyright (C) 2021 Nick Weiler, Justus Krebs, Franz Freitag
 * Copyright (C) 2021 Otto-von-Guericke Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_rp2040
 * @ingroup     drivers_periph_uart
 * @{
 *
 * @file
 * @brief       UART driver implementation for the RP2040
 *
 * @author      Franz Freitag <franz.freitag@st.ovgu.de>
 * @author      Justus Krebs <justus.krebs@st.ovgu.de>
 * @author      Nick Weiler <nick.weiler@st.ovgu.de>
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include "board.h"
#include "bitarithm.h"
#include "irq.h"
#include "mutex.h"
#include "periph/gpio.h"
#include "periph/uart.h"
#include "periph_cpu.h"
#include "periph_conf.h"
#include "reg_atomic.h"

#include "assert.h"

static uart_isr_ctx_t ctx[UART_NUMOF];
static mutex_t tx_lock = MUTEX_INIT_LOCKED;

void _irq_enable(uart_t uart)
{
    UART0_Type *dev = uart_config[uart].dev;
    unsigned irq_state = irq_disable();
    /* register callback */
    uint32_t irqs = UART0_UARTIMSC_TXIM_Msk;
    if (ctx[uart].rx_cb) {
        irqs |= UART0_UARTIMSC_RXIM_Msk;
    }
    dev->UARTIMSC.reg = irqs;

    irq_restore(irq_state);
    NVIC_EnableIRQ(uart_config[uart].irqn);
}

void _set_symbolrate(uart_t uart, uint32_t baud)
{
    assert(baud != 0);
    UART0_Type *dev = uart_config[uart].dev;
    uint32_t baud_rate_div = (8 * CLOCK_CLKPERI / baud);
    uint32_t baud_ibrd = baud_rate_div >> 7;
    uint32_t baud_fbrd;

    if (baud_ibrd == 0) {
        baud_ibrd = 1;
        baud_fbrd = 0;
    }
    else if (baud_ibrd >= 65535) {
        baud_ibrd = 65535;
        baud_fbrd = 0;
    }
    else {
        baud_fbrd = ((baud_rate_div & 0x7f) + 1) / 2;
    }

    reg_atomic_set(&(dev->UARTIBRD.reg), baud_ibrd & UART0_UARTIBRD_BAUD_DIVINT_Msk);
    reg_atomic_set(&(dev->UARTFBRD.reg), baud_fbrd & UART0_UARTFBRD_BAUD_DIVFRAC_Msk);

    reg_atomic_set(&(dev->UARTLCR_H.reg), 0);
}

int uart_mode(uart_t uart, uart_data_bits_t data_bits, uart_parity_t uart_parity,
              uart_stop_bits_t stop_bits)
{
    UART0_Type *dev = uart_config[uart].dev;

    uint32_t config = (data_bits << UART0_UARTLCR_H_WLEN_Pos);

    switch (uart_parity) {
    case UART_PARITY_NONE:
        break;
    case UART_PARITY_EVEN:
        config |= UART0_UARTLCR_H_EPS_Msk | UART0_UARTLCR_H_PEN_Msk;
        break;
    case UART_PARITY_ODD:
        config |= UART0_UARTLCR_H_PEN_Msk;
        break;
    default:
        return UART_NOMODE;
    }

    if (stop_bits == UART_STOP_BITS_2) {
        config |= UART0_UARTLCR_H_STP2_Msk;
    }

    dev->UARTLCR_H.reg = config;

    return UART_OK;
}

void uart_init_pins(uart_t uart)
{
    gpio_init(uart_config[uart].tx_pin, GPIO_OUT);
    volatile gpio_io_ctrl_t *io_config_reg_tx = gpio_io_register(uart_config[uart].tx_pin);
    io_config_reg_tx->function_select = FUNCTION_SELECT_UART;
    if (ctx[uart].rx_cb) {
        gpio_init(uart_config[uart].rx_pin, GPIO_IN_PU);
        volatile gpio_io_ctrl_t *io_config_reg_rx = gpio_io_register(uart_config[uart].rx_pin);
        io_config_reg_rx->function_select = FUNCTION_SELECT_UART;
    }
}

void uart_deinit_pins(uart_t uart)
{
    *gpio_io_register_u32(uart_config[uart].tx_pin) = 0;
    *gpio_pad_register_u32(uart_config[uart].tx_pin) = 0;
    SIO->GPIO_OE_CLR.reg = 1LU << uart_config[uart].tx_pin;
    if (ctx[uart].rx_cb) {
        *gpio_io_register_u32(uart_config[uart].rx_pin) = 0;
        *gpio_pad_register_u32(uart_config[uart].rx_pin) = 0;
    }
}

void uart_poweron(uart_t uart)
{
    uint32_t reset_bit_mask = (uart) ? RESETS_RESET_uart1_Msk : RESETS_RESET_uart0_Msk;
    periph_reset(reset_bit_mask);
    periph_reset_done(reset_bit_mask);
}

void uart_poweroff(uart_t uart)
{
    uart_deinit_pins(uart);
    periph_reset((uart) ? RESETS_RESET_uart1_Msk : RESETS_RESET_uart0_Msk);
}

int uart_init(uart_t uart, uint32_t baud, uart_rx_cb_t rx_cb, void *arg)
{
    if (uart >= UART_NUMOF) {
        return UART_NODEV;
    }

    UART0_Type *dev = uart_config[uart].dev;
    ctx[uart].rx_cb = rx_cb;
    ctx[uart].arg = arg;

    uart_poweron(uart);
    _set_symbolrate(uart, baud);

    if (uart_mode(uart, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1) != UART_OK) {
        return UART_NOMODE;
    }

    /* enable UART */
    reg_atomic_set(&dev->UARTCR.reg, UART0_UARTCR_UARTEN_Msk
                                        | UART0_UARTCR_RXE_Msk
                                        | UART0_UARTCR_TXE_Msk);
    uart_init_pins(uart);

    _irq_enable(uart);

    return UART_OK;
}

void uart_write(uart_t uart, const uint8_t *data, size_t len)
{
    UART0_Type *dev = uart_config[uart].dev;

    for (size_t i = 0; i < len; i++) {
        dev->UARTDR.reg = data[i];
        mutex_lock(&tx_lock);
    }
}



void isr_handler(uint8_t num)
{
    UART0_Type *dev = uart_config[num].dev;

    uint32_t status = dev->UARTMIS.reg;
    dev->UARTICR.reg = status;

    if (status & UART0_UARTMIS_TXMIS_Msk) {
        mutex_unlock(&tx_lock);
    }

    if (status & UART0_UARTMIS_RXMIS_Msk) {
        ctx[num].rx_cb(ctx[num].arg, (uint8_t)dev->UARTDR.reg);
    }
}

void isr_uart0(void)
{
    isr_handler(0);
}

void isr_uart1(void)
{
    isr_handler(1);
}
