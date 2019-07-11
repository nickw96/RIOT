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
#ifdef MODULE_AUTO_INIT_SAUL
#include "saul_reg.h"
#endif /* MODULE_AUTO_INIT_SAUL */

/**
 * @brief   Define the number of configured sensors
 */
#define PMS5003_NUM     (sizeof(pms5003_params) / sizeof(pms5003_params[0]))

/**
 * @brief   Allocate memory for the device descriptors
 */
pms5003_dev_t pms5003_devs[PMS5003_NUM];

#ifdef MODULE_AUTO_INIT_SAUL

/**
 * @brief   Memory for the SAUL registry entries
 */
static saul_reg_t saul_entries[PMS5003_NUM];

/**
 * @brief   Define the number of saul info
 */
#define PMS5003_INFO_NUM (sizeof(pms5003_saul_info) / sizeof(pms5003_saul_info[0]))

/**
 * @name    Import SAUL endpoints
 * @{
 */
extern const saul_driver_t pms5003_saul_pm_driver;
/** @} */

#endif /* MODULE_AUTO_INIT_SAUL */

void auto_init_pms5003(void)
{
    for (unsigned int i = 0; i < PMS5003_NUM; i++) {
        LOG_DEBUG("[auto_init_saul] initializing pms5003 #%u\n", i);

        if (pms5003_init(&pms5003_devs[i], &pms5003_params[i])) {
            LOG_ERROR("[auto_init_saul] error initializing pms5003 #%u\n", i);
            continue;
        }

#ifdef MODULE_AUTO_INIT_SAUL
        saul_entries[i].dev = (void *)i;
        saul_entries[i].name = pms5003_saul_info[i].name;
        saul_entries[i].driver = &pms5003_saul_pm_driver;
        saul_reg_add(&(saul_entries[i]));
#endif /* MODULE_AUTO_INIT_SAUL */
    }
}

#else
typedef int dont_be_pedantic;
#endif /* MODULE_PMS5003 */
