/*
 * Copyright (C) 2020 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @defgroup    drivers_adc_ng  Common ADC API
 * @ingroup     drivers_periph
 *
 * This module contains a platform and hardware independent ADC API.
 * It is intended to address both advanced and simple use cases and allow
 * using both external and internal ADCs transparently.
 *
 * @{
 *
 * @file
 * @brief       Interface definition of the common ADC API
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef ADC_NG_H
#define ADC_NG_H

#include <assert.h>
#include <errno.h>
#include <stdint.h>

#include "adc_ng_internal.h"
#include "bitarithm.h"
#include "periph_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Pass this special value as parameter `ref` in @ref adc_ng_init
 *          to select the highest supported reference voltage
 */
#define ADC_NG_MAX_REF                  (0U)

/**
 * @name    Helper functions query supported ADC resolutions
 *
 * @{
 */

/**
 * @brief   Check if the given ADC supports the given resolution
 *
 * @param[in]       adc     ADC device to check
 * @param[in]       res     Resolution to check
 *
 * @retval  1               Resolution is supported
 * @retval  0               Resolution is not supported
 */
static inline int adc_ng_supports_res(uint8_t adc, uint8_t res)
{
    assert(adc < ADC_NG_NUMOF);
    assert(res < 32);
    return ((res < 32) && (adc_ng_drivers[adc]->res_supported & (1 << res)));
}

/**
 * @brief   Get the highest supported resolution of an ADC
 *
 * @param[in]       adc     ADC to get the highest supported resolution of
 */
static inline uint8_t adc_ng_max_res(uint8_t adc)
{
    assert(adc < ADC_NG_NUMOF);
    return (uint8_t)bitarithm_msb(adc_ng_drivers[adc]->res_supported);
}

/**
 * @brief   Get the highest supported resolution of an ADC
 *
 * @param[in]       adc     ADC to get the highest supported resolution of
 */
static inline uint8_t adc_ng_min_res(uint8_t adc)
{
    assert(adc < ADC_NG_NUMOF);
    return (uint8_t)bitarithm_lsb(adc_ng_drivers[adc]->res_supported);
}

/** @} */

/**
 * @brief   Initialize and power up the ADC channel @p chan of device @p adc
 *
 * @param[in]       adc     ADC device to initialize a channel of
 * @param[in]       chan    Channel to initialize
 * @param[in]       res     Resolution to sample at
 * @param[in,out]   ref     Reference voltage in mV to use (***see details!***)
 *
 * @retval  0               Success
 * @retval  -ENOTSUP        Requested resolution not supported
 * @retval  -ENXIO          No such channel
 * @retval  -ERANGE         Requested reference voltage is higher than all
 *                          available references
 * @retval  -EALREADY       The ADC is already powered and configured
 * @retval  <0              A driver specific error occurred
 *
 * @note    On success, the ADC is powered up and consumes additional power
 *          until @ref adc_ng_off is called.
 *
 * The reference voltage to use is given with @p ref in millivolt. The driver
 * will pick a reference voltage that is as close to @p ref as possible, but
 * not smaller. The actually chosen reference voltage is stored in @p ref.
 */
int adc_ng_init(uint8_t adc, uint8_t chan, uint8_t res, uint16_t *ref);

/**
 * @brief   Turn of the given ADC device
 *
 * @param[in]       adc     ADC device to turn off
 */
static inline void adc_ng_off(uint8_t adc)
{
    assert(adc < ADC_NG_NUMOF);
    adc_ng_drivers[adc]->off(adc_ng_handles[adc]);
}

/**
 * @name    Functions access to access the raw ADC output
 *
 * These functions return the binary value returned by the ADC, rather than
 * meaningful physical units.
 *
 * @{
 */

/**
 * @brief   Perform a single conversion using the specified ADC channel
 *
 * @param[in]       adc     ADC device to use
 * @param[out]      dest    The result is stored here
 *
 * @pre     The ADC @p adc is initialized using @ref adc_ng_init
 *
 * @retval  0               Success
 * @retval  <0              A device specific error occurred
 */
static inline int adc_ng_single(uint8_t adc, uint32_t *dest)
{
    assert(adc < ADC_NG_NUMOF);
    assert(dest);
    return adc_ng_drivers[adc]->single(adc_ng_handles[adc], dest);
}

/**
 * @brief   Perform a burst conversion using the specified ADC
 *
 * @param[in]       adc     ADC device to use
 * @param[out]      dest    The results are stored here
 * @param[in]       num     The number of conversions to perform
 *
 * @pre     The ADC @p adc is initialized using @ref adc_ng_init
 *
 * @retval  0               Success
 * @retval  <0              A device specific error occurred
 *
 * With `USEMODULE += adc_ng_burst`, some ADC drivers might provide a highly
 * efficient implementation e.g. using DMA. If either the driver does not
 * provide such implementation, or the module `adc_ng_burst` is not used, a
 * slower but ROM-efficient fallback implementation is used instead.
 */
int adc_ng_burst(uint8_t adc, uint32_t *dest, size_t num);

/**
 * @brief   Initialize an ADC channel, perform a single conversion with
 *          maximum resolution and range, and power it off again
 *
 * @param[in]       adc     ADC device to use
 * @param[in]       chan    Channel to use
 * @param[out]      dest    The result is stored here
 *
 * @retval  0               Success
 * @retval  <0              A driver specific error occurred
 *
 * Refer to the documentation of @ref adc_init for details on @p ref
 */
static inline int adc_ng_quick(uint8_t adc, uint8_t chan,
                               uint32_t *dest)
{
    uint16_t ref = ADC_NG_MAX_REF;
    int retval = adc_ng_init(adc, chan, adc_ng_max_res(adc), &ref);
    if (retval) {
        return retval;
    }
    retval = adc_ng_single(adc, dest);
    adc_ng_off(adc);
    return retval;
}

/** @} */

/**
 * @name    Functions to get measurements in voltage levels
 *
 * @{
 */

/**
 * @brief   Run a single measurements and get the result in mV
 *
 * @param[in]       adc     ADC to use
 * @param[out]      dest_mv Write the measurement result in mV here
 *
 * @pre     The state given in @p state has been initialized (see
 *          @ref adc_ng_init) and the ADC currently is powered
 *
 * @retval  0               Success
 * @retval  <0              A driver specific error occurred
 *
 * @note    Please note that a while a precision roughly in the order of µV
 *          is with the right setup and ADC technically feasible, the accuracy
 *          of the result is more likely in the order of mV.
 */
static inline int adc_ng_voltage(uint8_t adc, uint16_t *dest_mv)
{
    uint32_t sample;
    int retval = adc_ng_single(adc, &sample);
    if (retval) {
        return retval;
    }

    *dest_mv = adc_ng_convert(adc, sample);
    return 0;
}
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ADC_NG_H */
/** @} */
