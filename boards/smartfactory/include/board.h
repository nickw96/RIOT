/*
 * Copyright (C) 2015 TriaGnoSys GmbH
 *               2017 Alexander Kurth, Sören Tempel, Tristan Bruns
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     boards_bluepill_128kib
 *
 * This board can be bought very cheaply on sides like eBay or
 * AliExpress. Although the MCU nominally has 64 KiB ROM, most of them
 * have 128 KiB ROM. For more information see:
 * https://web.archive.org/web/20190527040051/http://wiki.stm32duino.com/index.php?title=Blue_Pill
 *
 * @{
 *
 * @file
 * @brief       Peripheral MCU configuration for the bluepill board
 *
 * @author      Víctor Ariño <victor.arino@triagnosys.com>
 * @author      Sören Tempel <tempel@uni-bremen.de>
 * @author      Tristan Bruns <tbruns@uni-bremen.de>
 * @author      Alexander Kurth <kurth1@uni-bremen.de>
 */

#ifndef BOARD_H
#define BOARD_H

#include "board_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Board common contains all required info */

#define WS281X_PARAM_PIN        GPIO_PIN(PORT_B, 5)
#define WS281X_PARAM_NUMOF      4

#define CC110X_PARAM_CS         GPIO_PIN(PORT_A, 4)
#define CC110X_PARAM_GDO0       GPIO_PIN(PORT_B, 3)
#define CC110X_PARAM_GDO2       GPIO_PIN(PORT_B, 4)
#define CC110X_PARAM_PATABLE    (&cc110x_patable_433mhz)
#define CC110X_PARAM_CONFIG     (&cc110x_config_433mhz_250kbps_300khz)
#define CC110X_PARAM_CHANNELS   (&cc110x_chanmap_433mhz_300khz)

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
/** @} */
