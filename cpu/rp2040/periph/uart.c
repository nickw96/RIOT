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
#include "mutex.h"
#include "periph/gpio.h"
#include "periph/uart.h"
#include "periph_cpu.h"
#include "periph_conf.h"
#include "reg_atomic.h"

#include "assert.h"

static uart_isr_ctx_t ctx[UART_NUMOF];

//static mutex_t tx_lock = MUTEX_INIT;

void _uart_baudrate(uart_t uart, uint32_t baudrate) {
    assert(baudrate != 0);
    UART0_Type *dev = uart_config[uart].dev;
    uint32_t baud_rate_div = (8 * CLOCK_CLKPERI / baudrate);
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
                uart_stop_bits_t stop_bits) {
    UART0_Type *dev = uart_config[uart].dev;

    reg_atomic_clear(&(dev->UARTLCR_H.reg), UART0_UARTLCR_H_WLEN_Msk
                                                | UART0_UARTLCR_H_STP2_Msk
                                                | UART0_UARTLCR_H_EPS_Msk
                                                | UART0_UARTLCR_H_PEN_Msk);

    switch (uart_parity) {
        case UART_PARITY_NONE:
            break;
        case UART_PARITY_EVEN:
            reg_atomic_set(&(dev->UARTLCR_H.reg), UART0_UARTLCR_H_EPS_Msk
                                                    | UART0_UARTLCR_H_PEN_Msk);
            break;
        case UART_PARITY_ODD:
            reg_atomic_set(&(dev->UARTLCR_H.reg), UART0_UARTLCR_H_PEN_Msk);
            break;
        default:
            return UART_NOMODE;
    }

    reg_atomic_set(&(dev->UARTLCR_H.reg), UART0_UARTLCR_H_WLEN_Msk & data_bits);
    reg_atomic_set(&(dev->UARTLCR_H.reg), UART0_UARTLCR_H_STP2_Msk & stop_bits);

    return UART_OK;
}

void uart_init_pins(uart_t uart) {
    gpio_init(uart_config[uart].tx_pin, GPIO_OUT);
    volatile gpio_io_ctrl_t *io_config_reg_tx = gpio_io_register(uart_config[uart].tx_pin);
    io_config_reg_tx->function_select = FUNCTION_SELECT_UART;
    if(ctx[uart].rx_cb) {
        gpio_init(uart_config[uart].rx_pin, GPIO_IN_PU);
        volatile gpio_io_ctrl_t *io_config_reg_rx = gpio_io_register(uart_config[uart].rx_pin);
        io_config_reg_rx->function_select = FUNCTION_SELECT_UART;
    }
}

void uart_deinit_pins(uart_t uart) {
    volatile gpio_io_ctrl_t *io_config_reg_tx = gpio_io_register(uart_config[uart].tx_pin);
    io_config_reg_tx->function_select = FUNCTION_SELECT_SIO;
    if(ctx[uart].rx_cb) {
        volatile gpio_io_ctrl_t *io_config_reg_rx = gpio_io_register(uart_config[uart].rx_pin);
        io_config_reg_rx->function_select = FUNCTION_SELECT_SIO;
    }
}

void uart_poweron(uart_t uart) {
    uint32_t reset_bit_mask = (uart) ? RESETS_RESET_uart1_Msk : RESETS_RESET_uart0_Msk;
    reg_atomic_set(&(RESETS->RESET.reg), reset_bit_mask);
    reg_atomic_clear(&(RESETS->RESET.reg), reset_bit_mask);

    while(~(RESETS->RESET_DONE.reg) & reset_bit_mask) { /*wait until it's done */ }
}

void uart_poweroff(uart_t uart) {
    uart_deinit_pins(uart);
    uint32_t reset_bit_mask = (uart) ? RESETS_RESET_uart1_Msk : RESETS_RESET_uart0_Msk;
    reg_atomic_set(&(RESETS->RESET.reg), reset_bit_mask);
}

int uart_init(uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg) {
    if (uart >= UART_NUMOF) {
        return UART_NODEV;
    }

    UART0_Type *dev = uart_config[uart].dev;

    /* register callback */
    if (rx_cb) {
        ctx[uart].rx_cb = rx_cb;
        ctx[uart].arg = arg;
        reg_atomic_set(&(dev->UARTIMSC.reg), UART0_UARTIMSC_RXIM_Msk);
    }

    //reg_atomic_set(&(dev->UARTIMSC.reg), UART0_UARTIMSC_TXIM_Msk);

    uart_poweron(uart);

    _uart_baudrate(uart, baudrate);    

    if( uart_mode(uart, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1) == UART_NOMODE )
        return UART_NOMODE;

    reg_atomic_set(&(dev->UARTCR.reg), UART0_UARTCR_UARTEN_Msk 
                                        | UART0_UARTCR_RXE_Msk 
                                        | UART0_UARTCR_TXE_Msk);
    reg_atomic_set(&(dev->UARTLCR_H.reg), UART0_UARTLCR_H_FEN_Msk);
    reg_atomic_set(&(dev->UARTDMACR.reg), UART0_UARTDMACR_TXDMAE_Msk 
                                            | UART0_UARTDMACR_RXDMAE_Msk);
    uart_init_pins(uart);

    return UART_OK;
}

void uart_write(uart_t uart, const uint8_t *data, size_t len) {
    UART0_Type *dev = uart_config[uart].dev;
    
    for (size_t i = 0; i < len; ++i) {
        while (dev->UARTFR.bit.TXFF) { }
        /* if (dev->UARTFR.bit.TXFF) {
            mutex_lock(&tx_lock);
        } */
        reg_atomic_set(&(dev->UARTDR.reg), *data++ | UART0_UARTDR_DATA_Msk);        
    }
}

void isr_handler(uint8_t num) {
    UART0_Type *dev = uart_config[num].dev;

    /*if (dev->UARTMIS.bit.TXMIS) {
        mutex_unlock(&tx_lock);
        reg_atomic_set(&(dev->UARTICR.reg), UART0_UARTICR_TXIC_Msk);
    }
    else */ if (dev->UARTMIS.bit.RXMIS) {
        ctx[num].rx_cb(ctx[num].arg, (uint8_t) (dev->UARTDR.bit.DATA));
        reg_atomic_set(&(dev->UARTICR.reg), UART0_UARTICR_RXIC_Msk);
    }
}

void isr_uart0(void) {
    isr_handler(0);
}

void isr_uart1(void) {
    isr_handler(1);
}