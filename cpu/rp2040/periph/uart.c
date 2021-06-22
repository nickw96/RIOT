/*
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
 *
 * @}
 */

#include "board.h"
#include "bitarithm.h"
#include "periph/gpio.h"
#include "periph/uart.h"
#include "periph_cpu.h"
#include "periph_conf.h"

static uart_isr_ctx_t ctx[UART_NUMOF];

int uart_mode(uart_t uart, uart_data_bits_t data_bits, uart_parity_t uart_parity,
                uart_stop_bits_t stop_bits) {
    UART0_Type *dev = (UART0_Type *) (uart_config[uart].dev);
    
    switch (uart_parity) {
        case UART_PARITY_NONE:
            dev->UARTLCR_H.bit.PEN = 0;
            dev->UARTLCR_H.bit.EPS = 0;
            break;
        case UART_PARITY_EVEN:
            dev->UARTLCR_H.bit.PEN = 1;
            dev->UARTLCR_H.bit.EPS = 1;
            break;
        case UART_PARITY_ODD:
            dev->UARTLCR_H.bit.PEN = 1;
            dev->UARTLCR_H.bit.EPS = 0;
            break;
        default:
            return UART_NOMODE;
    }

    dev->UARTLCR_H.bit.WLEN = data_bits;
    dev->UARTLCR_H.bit.STP2 = stop_bits;

    return UART_OK;
}

void uart_init_pins(uart_t uart) {
    gpio_init(uart_config[uart].tx_pin, GPIO_OUT);
    if(ctx[uart].rx_cb) {
        gpio_init(uart_config[uart].rx_pin, GPIO_IN_PU);
    }
}

int uart_init(uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg) {
    if (uart >= UART_NUMOF) {
        return UART_NODEV;
    }

    UART0_Type *dev = (UART0_Type *) (uart_config[uart].dev);

    /* register callback */
    ctx[uart].rx_cb = rx_cb;
    ctx[uart].arg = arg;

    unsigned long reset_bit_mask = (uart) ? RESETS_RESET_uart1_Msk : RESETS_RESET_uart0_Msk;
    RESETS->RESET |= reset_bit_mask;
    RESETS->RESET &=  ~(reset_bit_mask);

    while(~(RESETS->RESET_DONE) & reset_bit_mask) { /*wait until it's done */ }

    /* TODO clk*/
    /* TODO enable crlf support*/
    /* TODO configure baudrate */

    if( uart_mode(uart, UART_DATA_BITS_8, UART_PARITIY_NONE, UART_STOP_BITS_1) == UART_NOMODE )
        return UART_NOMODE;

    dev->UARTCR |= UART0_UARTCR_UARTEN_Msk | UART0_UARTCR_RXE_Msk | UART0_UARTCR_TXE_Msk;
    dev->UARTLCR_H |= UART0_UARTLCR_H_FEN_Msk;
    dev->UARTDMACR |= UART0_UARTDMACR_TXDMAE_Msk | UART0_UARTDMACR_RXDMAE_Msk;

    /* TODO uart_init_pins*/

    return UART_OK;
}

void uart_write(uart_t uart, const uint8_t *data, size_t len) {
    UART0_Type *dev = (UART0_Type *) (uart_config[uart].dev);
    
    for (size_t i = 0; i < len; ++i) {
        while (dev->UARTFR & UART0_UARTFR_TXFF_Msk) { }
        dev->UARTDR.bit.DATA = *data++;        
    }
}

/* TODO uart_poweron/poweroff, isr_handler */