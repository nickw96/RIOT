/*
 * Copyright (C) 2020 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_adc_ng
 * @{
 *
 * @file
 * @brief       Implementation of common functions of the ADC API
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 * @}
 */

#include <errno.h>

#include "adc_ng.h"

#if (ADC_NG_NUMOF == 1) && defined(MODULE_PERIPH_ADC_NG)
extern const adc_ng_driver_t periph_adc_ng_driver;
void * const adc_ng_handles[ADC_NG_NUMOF] = { NULL };
const adc_ng_driver_t * const adc_ng_drivers[ADC_NG_NUMOF] = {
    &periph_adc_ng_driver
};
#endif
/* selected reference voltage in mV */
uint16_t adc_ng_refs[ADC_NG_NUMOF];
/* selected resolution */
uint8_t adc_ng_res[ADC_NG_NUMOF];

int adc_ng_init(uint8_t adc, uint8_t chan, uint8_t res, uint16_t *ref)
{
    assert(adc < ADC_NG_NUMOF);
    assert(ref);
    const adc_ng_driver_t *drv = adc_ng_drivers[adc];
    void *handle = adc_ng_handles[adc];

    if (!(drv->res_supported & (1 << res))) {
        return -ENOTSUP;
    }

    uint8_t idx = 0;

    if (*ref == ADC_NG_MAX_REF) {
        while (drv->refs[++idx]) { }
        idx--;
    }
    else {
        while ((drv->refs[idx] & ADC_NG_REF_MASK) < *ref) {
            if (!drv->refs[++idx]) {
                /* No reference big enough to fit value given in *ref */
                return -ERANGE;
            }
        }
    }

    adc_ng_refs[adc] = *ref = drv->refs[idx] & ADC_NG_REF_MASK;
    adc_ng_res[adc] = res;

    return drv->init(handle, chan, res, (uint8_t)idx);
}

uint16_t adc_ng_convert(uint8_t adc, uint32_t sample)
{
    assert(adc < ADC_NG_NUMOF);
    /* V_in = (sample * V_ref) / (2^resolution) */
    uint64_t vin = sample;
    vin *= adc_ng_refs[adc];
    vin >>= adc_ng_res[adc];
    return (uint16_t)vin;
}

int adc_ng_burst(uint8_t adc, uint32_t *dest, size_t num)
{
    assert(adc < ADC_NG_NUMOF);
    assert(dest && num);
    const adc_ng_driver_t *drv = adc_ng_drivers[adc];
    void * handle = adc_ng_handles[adc];
#ifdef MODULE_ADC_BURST
    if (drv->burst) {
        return drv->burst(handle, dest, num);
    }
    else
#endif
    {
        int retval;
        for (size_t i = 0; i < num; i++) {
            retval = drv->single(handle, &dest[i]);
            if (retval) {
                return retval;
            }
        }
    }

    return 0;
}

int adc_ng_measure_ref(uint8_t adc, uint8_t ref_idx, uint16_t *dest)
{
    assert(adc < ADC_NG_NUMOF);
    assert(dest);
    const adc_ng_driver_t *drv = adc_ng_drivers[adc];
    void *handle = adc_ng_handles[adc];
    /* It makes no sense to use the same voltage source as reference and input */
    assert(drv->fixed_ref_input != ref_idx);

    int retval;
    uint32_t sample;
    uint8_t res_max = adc_ng_max_res(adc);
    retval = drv->init(handle, ADC_NG_CHAN_FIXED_REF, res_max, ref_idx);
    if (retval) {
        return retval;
    }
    retval = drv->single(handle, &sample);
    drv->off(handle);
    if (retval) {
        return retval;
    }
    /*
     * The sample s with the resolution r has the value:
     *
     *     s = (V_in * 2^r) / V_ref
     *
     * In this case we're interested in V_ref and know V_in, so:
     *
     *     V_ref = (V_in * 2^r) / s
     */
    uint64_t vref = (1ULL << res_max);
    vref *= drv->refs[drv->fixed_ref_input];
    vref += sample >> 1; /* <- Scientific rounding */
    vref /= sample;
    *dest = (uint16_t)vref;
    return 0;
}
