/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
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
 * @brief       Test application for the MFRC522 driver
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "shell.h"
#include "mfrc522.h"
#include "mfrc522_params.h"

int main(void)
{
    puts("MFRC522 driver test application\n"
         "===============================\n");

    mfrc522_t m;
    int retval = mfrc522_init(&m, &mfrc522_params[0]);
    if (retval) {
        printf("mfrc522_init() failed with %d\n", retval);
    }
    else {
        mfrc522_scan_start(&m, NULL, NULL);
    }

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
