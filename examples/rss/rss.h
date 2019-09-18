/*
 * Copyright (C) 2019 Marian Buschsieweke
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @defgroup    examples_rss
 * @ingroup     examples
 *
 * @{
 *
 * @file
 * @brief       Forward declarations needed in the RIOT Sound System example
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef RSS_H
#define RSS_H

#include <stdint.h>

#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Thread controlling the DFPlayer Mini and the Neopixel
 *
 * @param   arg     Unused argument, set to `NULL`.
 *
 * @return  Returns `NULL` on failure, doesn't return at all on success
 */
void * control_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* CC110X_H */
/** @} */
