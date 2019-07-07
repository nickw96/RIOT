/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

/*
 * @ingroup     sys_auto_init_saul
 * @{
 *
 * @file
 * @brief       Auto initialization for the PMS5003 particulate matter sensor
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#ifdef MODULE_PMS5003

#define ENABLE_DEBUG        (0)

#include "assert.h"
#include "log.h"
#include "pms5003_params.h"
#include "pms5003.h"

/**
 * @brief   Define the number of configured sensors
 */
#define PMS5003_NUM     (sizeof(pms5003_params) / sizeof(pms5003_params[0]))

/**
 * @brief   Allocate memory for the device descriptors
 */
pms5003_dev_t pms5003_devs[PMS5003_NUM];

void auto_init_pms5003(void)
{
    for (unsigned int i = 0; i < PMS5003_NUM; i++) {
        LOG_DEBUG("[auto_init_saul] initializing pms5003 #%u\n", i);

        if (pms5003_init(&pms5003_devs[i], &pms5003_params[i])) {
            LOG_ERROR("[auto_init_saul] error initializing pms5003 #%u\n", i);
            continue;
        }
    }
}

#else
typedef int dont_be_pedantic;
#endif /* MODULE_PMS5003 */
