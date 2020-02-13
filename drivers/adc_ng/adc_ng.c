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
const adc_ng_backend_t adc_ng_backends[ADC_NG_NUMOF] = {
    {
        .driver = &periph_adc_ng_driver,
        .handle = NULL
    }
};
#endif
/* selected reference voltage in mV */
int16_t adc_ng_refs[ADC_NG_NUMOF];
/* selected resolution */
uint8_t adc_ng_res[ADC_NG_NUMOF];

int adc_ng_init(uint8_t adc, uint8_t chan, uint8_t res, int16_t *ref)
{
    assert(adc < ADC_NG_NUMOF);
    assert(ref);
    assert((res > 0) && (res <= 32));
    const adc_ng_backend_t be = adc_ng_backends[adc];

    if (!(be.driver->res_supported & (1 << (res - 1)))) {
        return -ENOTSUP;
    }

    uint8_t idx = 0;

    if (*ref == ADC_NG_MAX_REF) {
        while (be.driver->refs[++idx]) { }
        idx--;
    }
    else {
        if ((*ref < 0) && (*ref < be.driver->refs[0])) {
            /* No reference small enough to cover the range [*ref; 0] */
            return -ERANGE;
        }
        /* Positive reference voltage: Select the lowest reference that
         * is equal to or greater than the requested reference */
        while (be.driver->refs[idx] < *ref) {
            if (!be.driver->refs[++idx]) {
                if (*ref < 0) {
                    idx--;
                    break;
                }
                /* No reference big enough to cover the range [0; ref] */
                return -ERANGE;
            }
        }
    }

    adc_ng_refs[adc] = *ref = be.driver->refs[idx];
    adc_ng_res[adc] = res;

    return be.driver->init(be.handle, chan, res, (uint8_t)idx);
}

int16_t adc_ng_convert(uint8_t adc, int32_t sample)
{
    assert(adc < ADC_NG_NUMOF);
    /* V_in = (sample * V_ref) / (2^resolution - 1) */
    int64_t vin = sample;
    vin *= adc_ng_refs[adc];
    vin /= (1 << adc_ng_res[adc]) - 1;
    return (int16_t)vin;
}

int adc_ng_burst(uint8_t adc, int32_t *dest, size_t num)
{
    assert(adc < ADC_NG_NUMOF);
    assert(dest && num);
    const adc_ng_backend_t be = adc_ng_backends[adc];
#ifdef MODULE_ADC_BURST
    if (be.driver->burst) {
        return be.driver->burst(be.handle, dest, num);
    }
    else
#endif
    {
        int retval;
        for (size_t i = 0; i < num; i++) {
            retval = be.driver->single(be.handle, &dest[i]);
            if (retval) {
                return retval;
            }
        }
    }

    return 0;
}

int adc_ng_measure_ref(uint8_t adc, uint8_t ref_idx, int16_t *dest)
{
    assert(adc < ADC_NG_NUMOF);
    assert(dest);
    const adc_ng_backend_t be = adc_ng_backends[adc];
    /* It makes no sense to use the same voltage source as reference and input */
    assert(be.driver->ref_input_idx != ref_idx);

    int retval;
    int32_t sample;
    uint8_t res_max = adc_ng_max_res(adc);
    retval = be.driver->init(be.handle, ADC_NG_CHAN_FIXED_REF, res_max,
                             ref_idx);
    if (retval) {
        return retval;
    }
    retval = be.driver->single(be.handle, &sample);
    be.driver->off(be.handle);
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
    int64_t vref = (1ULL << res_max);
    vref *= be.driver->refs[be.driver->ref_input_idx];
    vref += sample >> 1; /* <- Scientific rounding */
    vref /= sample;
    *dest = (int16_t)vref;
    return 0;
}
