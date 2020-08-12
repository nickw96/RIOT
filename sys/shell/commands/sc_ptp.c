/*
 * Copyright (C) 2020 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_shell_commands
 * @{
 *
 * @file
 * @brief       Provides shell commands to access PTP clocks and the PTP client
 *              state
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "fmt.h"
#include "kernel_defines.h"
#include "net/ptp.h"
#include "periph/ptp.h"

const uint16_t days_table[] = {
    /*Jan  Feb  Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec */
        0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334
};

static unsigned leap_years_since_epoch(unsigned year)
{
    /* leap years since year 0 */
    unsigned leap_years = year / 4 - year / 100 + year / 400;
    /* compensate the 477 leap years between year 0 and year 1970 */
    return leap_years - 477;
};

static int is_leap_year(unsigned year)
{
    if ((year % 400) == 0) {
        return 1;
    }

    if ((year % 100) == 0) {
        return 0;
    }

    if ((year % 4) == 0) {
        return 1;
    }

    return 0;
}

static void print_timestamp(ptp_timestamp_t *ts)
{
    ldiv_t d = ldiv(ts->seconds, 60);
    unsigned sec = d.rem;
    d = ldiv(d.quot, 60);
    unsigned min = d.rem;
    d = ldiv(d.quot, 24);
    unsigned hour = d.rem;
    d = ldiv(d.quot, 365);
    unsigned day = d.rem;
    unsigned year = d.quot + 1970;
    unsigned leap_years = leap_years_since_epoch(year);
    if (day >= leap_years) {
        day -= leap_years;
    }
    else {
        day += 365 - leap_years;
        year--;
    }

    unsigned month;
    for (month = 1; month < 12; month++) {
        if (day < days_table[month]) {
            day -= days_table[month - 1];
            day++;
            break;
        }
    }

    if (is_leap_year(year)) {
        if (month <= 2) {
            /* undo compensation for current leap year, as the leap day has not
             * happened yet */
            day++;
            if ((month == 1) && (day == 32)) {
                month = 2;
                day = 1;
            }
        }
    }

    printf("%u-%u-%u %02u:%02u:%02u.%09lu\n",
           year, month, day, hour, min, sec, (unsigned long)ts->nanoseconds);
}

static void print_clock_id(const ptp_clock_id_t *clock_id)
{
    const uint8_t *id = clock_id->bytes;
    printf("%02x%02x%02x.%02x%02x.%02x%02x%02x\n",
           id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]);
}

int sc_ptp(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    ptp_timestamp_t ts;
    ptp_clock_read(&ts);
    printf("%s", "Current PTP time: ");
    print_timestamp(&ts);

    if (IS_USED(MODULE_PTP_CLIENT)) {
        ptp_clock_id_t server_clock;
        ptp_get_server_clock_id(&server_clock);
        printf("%s", "Local Clock ID:           ");
        print_clock_id(&ptp_local_clock_id);
        printf("%s", "Selected Server Clock ID: ");
        print_clock_id(&server_clock);
        printf("Current offset to UTC time: %u secs\n", (unsigned)ptp_get_utc_offset());
        printf("Estimated network delay (whole round trip): %lu ns\n",
               (unsigned long)ptp_get_rtt());
        int64_t drift = ptp_get_clock_drift();
        drift *= 1000000000LL;
        drift >>= 32;
        char sdrift[16];
        sdrift[fmt_s32_dfp(sdrift, (int32_t)drift, -7)] = '\0';
        printf("Estimated clock drift: %s%%\n", sdrift);
    }
    return 0;
}
