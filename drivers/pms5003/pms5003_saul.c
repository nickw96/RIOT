/*
 * Copyright 2019 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_pms5003
 * @{
 *
 * @file
 * @brief       SAUL adaption for PMS5003 devices
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "phydat.h"
#include "saul.h"
#include "pms5003.h"


static int read_pm(const void *dev, phydat_t *res)
{
    pms5003_data_t data;
    if (pms5003_read((pms5003_t)(size_t)dev, &data) == 0) {
        res->unit = UNIT_GPM3;
        res->scale = -6;
        res->val[0] = data.pm_1_0;
        res->val[1] = data.pm_2_5;
        res->val[2] = data.pm_10_0;
        return 3;
    }

    return -ECANCELED;
}

const saul_driver_t pms5003_saul_pm_driver = {
    .read = read_pm,
    .write = saul_notsup,
    .type = SAUL_SENSE_PM,
};

