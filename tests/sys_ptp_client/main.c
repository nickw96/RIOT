/*
 * Copyright (C) 2020 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Test application for the PTP client
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "kernel_defines.h"
#include "net/gnrc.h"
#include "net/gnrc/pktdump.h"
#include "periph/gpio.h"
#include "periph_cpu.h"
#include "shell.h"
#include "shell_commands.h"

#define MAIN_QUEUE_SIZE     (8)
#define _QUOTE(x)           #x
#define QUOTE(x)            _QUOTE(x)

#ifndef PPS_PIN
#define PPS_PIN     GPIO_PIN(PORT_G, 8)
#endif
#ifndef PPS_FREQ
#define PPS_FREQ    1
#endif

static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    puts("PTP client test application\n"
         "===========================\n"
         "\n"
         "Connect to a PTP server and use the \"ptp\" shell command to verify\n"
         "correct synchronization\n"
         "\n");

#if IS_USED(MODULE_STM32_ETH)
    puts("Enabling STM32 PTP-PPS signal at " QUOTE(PPS_FREQ) " Hz");
    stm32_eth_ptp_enable_pps(PPS_PIN, PPS_FREQ);
#endif
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
