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
 * @brief       Test application for the PMS5003 particulate matter sensor
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <errno.h>
#include <string.h>

#include "fmt.h"
#include "pms5003.h"
#include "shell.h"

static int dump_cmd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "dump", "Dump all PMS5003 measurements to the serial", dump_cmd },
    { NULL, NULL, NULL }
};

static const char spaces[16] = "                ";

static void print_col_u32_dec(uint32_t number, size_t width)
{
    char sbuf[10]; /* "4294967295" */
    size_t slen;

    slen = fmt_u32_dec(sbuf, number);
    if (width > slen) {
        width -= slen;
        while (width > sizeof(spaces)) {
            print(spaces, sizeof(spaces));
        }
        print(spaces, width);
    }
    print(sbuf, slen);
}

static void _cb_data(const pms5003_data_t *data, void *userdata)
{
    (void)userdata;
    print("|", 1);
    print_col_u32_dec(data->pm_1_0, 7);
    print("|", 1);
    print_col_u32_dec(data->pm_2_5, 7);
    print("|", 1);
    print_col_u32_dec(data->pm_10_0, 8);
    print("|", 1);
    print_col_u32_dec(data->pm_a_1_0, 7);
    print("|", 1);
    print_col_u32_dec(data->pm_a_2_5, 7);
    print("|", 1);
    print_col_u32_dec(data->pm_a_10_0, 8);
    print("|", 1);
    print_col_u32_dec(data->n_0_3, 7);
    print("|", 1);
    print_col_u32_dec(data->n_0_5, 7);
    print("|", 1);
    print_col_u32_dec(data->n_1_0, 7);
    print("|", 1);
    print_col_u32_dec(data->n_2_5, 7);
    print("|", 1);
    print_col_u32_dec(data->n_5_0, 7);
    print("|", 1);
    print_col_u32_dec(data->n_10_0, 6);
    print("|\n", 2);
}

static void _cb_error(pms5003_error_t error, void *userdata)
{
    (void)userdata;
    static const char *strs[PMS5003_ERROR_NUMOF] = {
        [PMS5003_NO_ERROR] = "No Error",
        [PMS5003_ERROR_CHECKSUM] = "Checksum Error",
        [PMS5003_ERROR_FORMAT] = "Format Error",
        [PMS5003_ERROR_TIMEOUT] = "Timeout Error",
    };

    const char *str = strs[error];
    if (!str) {
        str = "FIXME: Missing stringification for error!";
    }

    print_str(str);
    print("\n", 1);
}

static pms5003_callbacks_t cbs = {
    .cb_data = _cb_data,
    .cb_error = _cb_error,
};

static int dump_cmd(int argc, char **argv)
{
    if (argc != 2) {
        print_str("Usage: ");
        print_str(argv[0]);
        print_str("<1/0>\n");
        return 0;
    }

    if (!strcmp(argv[1], "1")) {
        print_str(
            "+------------------------+------------------------+----------------------------------------------+\n"
            "| Standard concentration | Atmospheric Environment|   # Particles in 0.1l air of diameter >=     |\n"
            "| PM1.0 | PM2.5 | PM10.0 | PM1.0 | PM2.5 | PM10.0 | 0.3µm | 0.5µm | 1.0µm | 2.5µm | 5.0µm | 10µm |\n"
            "+-------+-------+--------+-------+-------+--------+-------+-------+-------+-------+-------+------+\n"
        );
        pms5003_add_callbacks(0, &cbs);
    }
    else {
        pms5003_del_callbacks(0, &cbs);
        print_str("+-------+-------+--------+-------+-------+--------+-------+-------+-------+-------+-------+------+\n");
    }

    return 0;
}

int main(void)
{
    print_str(
        "PMS5003 Test Application\n"
        "========================\n"
        "\n"
        "Use the saul shell command to read data, or use \"dump 1\" to monitor\n"
        "the output of the sensor.\n"
    );

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
