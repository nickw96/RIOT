/*
 * Copyright 2019 Marian Buschsieweke
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples_rss
 * @{
 *
 * @file
 * @brief       RIOT Sound System example
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <stdio.h>

#include "shell.h"
#include "shell_commands.h"
#include "thread.h"

#include "rss.h"

static char control_thread_stack[THREAD_STACKSIZE_MAIN];

int main(void)
{
    if (thread_create(control_thread_stack, sizeof(control_thread_stack),
                      THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
                      control_thread, NULL, "rss_control") < 0) {
        puts("Failed to start control thread. Shit");
    }

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
