/*
 * Copyright (C) 2014-2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief       Test for low-level PWM drivers
 *
 * This test initializes the given PWM device to run at 1KHz with a 1000 step
 * resolution.
 *
 * The PWM is then continuously oscillating it's duty cycle between 0% to 100%
 * every 1s on every channel.
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Semjon Kerner <semjon.kerner@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "xtimer.h"
#include "shell.h"
#include "timex.h"
#include "periph/pwm.h"

#define OSC_INTERVAL    (10LU * US_PER_MS) /* 10 ms */
#define OSC_STEP        (10)
#define OSC_MODE        PWM_LEFT
#define OSC_FREQU       (1000U)
#define OSC_STEPS       (1000U)
#define PWR_SLEEP       (1U)

static uint32_t initiated;

static unsigned _get_dev(const char *dev_str)
{
    unsigned dev = atoi(dev_str);
    if (dev >= PWM_NUMOF) {
        printf("Error: device PWM_DEV(%u) is unknown\n", dev);
        return UINT_MAX;
    }
    return dev;
}

static int _init(int argc, char** argv)
{
    if (argc != 5) {
        printf("usage: %s <dev> <mode> <frequency> <resolution>\n", argv[0]);
        printf("\tdev: device by number between 0 and %u\n", PWM_NUMOF - 1);
        puts("\tmode:\n");
        puts("\t\t0: left aligned\n");
        puts("\t\t1: right aligned\n");
        puts("\t\t2: center aligned\n");
        puts("\tfrequency: desired frequency in Hz\n");
        puts("\tresolution: number between 2 and 65535");
        return 1;
    }

    unsigned dev = _get_dev(argv[1]);
    if (dev == UINT_MAX) {
        return 1;
    }

    pwm_mode_t pwm_mode;
    switch(atoi(argv[2])) {
        case(0):
            pwm_mode = PWM_LEFT;
            break;
        case(1):
            pwm_mode = PWM_RIGHT;
            break;
        case(2):
            pwm_mode = PWM_CENTER;
            break;
        default:
            printf("Error: mode %d is not supported.\n", atoi(argv[2]));
            return 1;
    }

    uint32_t pwm_freq = pwm_init(PWM_DEV(dev), pwm_mode,
                                 (uint32_t)atoi(argv[3]),
                                 (uint16_t)atoi(argv[4]));
    if (pwm_freq != 0) {
        printf("The pwm frequency is set to %" PRIu32 "\n", pwm_freq);
        initiated |= (1 << dev);
        return 0;
    }

    puts("Error: device is not initiated");
    return 1;
}

static int _set(int argc, char**argv)
{
    if (argc != 4) {
        printf("usage: %s <dev> <ch> <val>\n", argv[0]);
        printf("\tdev: device by number between 0 and %d\n", PWM_NUMOF - 1);
        puts("\tch: channel of device\n");
        puts("\tval: duty cycle\n");
        return 1;
    }

    unsigned dev = _get_dev(argv[1]);
    if (dev == UINT_MAX) {
        return 1;
    }

    if ((initiated & (1 << dev)) == 0) {
        puts("Error: pwm is not initiated.\n");
        puts("Execute init function first.\n");
        return 1;
    }

    uint8_t chan = atoi(argv[2]);
    if (chan >= pwm_channels(PWM_DEV(dev))) {
        printf("Error: channel %d is unknown.\n", chan);
        return 1;
    }

    pwm_set(PWM_DEV(dev), chan, (uint16_t)atoi(argv[3]));
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "init", "initial pwm configuration", _init },
    { "set", "set pwm duty cycle", _set },
    { NULL, NULL, NULL }
};

int main(void)
{
    puts("PWM peripheral driver test\n");
    initiated = 0;

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
