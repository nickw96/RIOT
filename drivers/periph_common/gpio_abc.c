/*
 * Copyright 2019 Marian Buschsieweke
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include "periph/gpio_abc.h"

#ifdef MODULE_PERIPH_GPIO_ABC

int gpio_abc_delay(uint16_t ns)
{
    if (ns + GPIO_ABC_MIN_ACCURACY < GPIO_ABC_OVERHEAD_NS) {
        return -1;
    }

    if (ns < GPIO_ABC_OVERHEAD_NS) {
        return 0;
    }

    return GPIO_ABC_DELAY(ns);
}

#else /* MODULE_PERIPH_GPIO_ABC */
typedef int dont_be_pedantic;
#endif /* MODULE_PERIPH_GPIO_ABC */
