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

#include "shell.h"
#include "shell_commands.h"

#include "net/gnrc/pktdump.h"
#include "net/gnrc.h"

#define MAIN_QUEUE_SIZE     (8)

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
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
