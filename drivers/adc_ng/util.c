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

#include "adc_ng_util.h"

int adc_ng_vcc(uint8_t adc, uint16_t *dest_mv)
{
    assert(adc < ADC_NG_NUMOF);
    assert(dest_mv);
    const adc_ng_driver_t *drv = adc_ng_drivers[adc];
    uint8_t vcc_idx = 0;
    while (!(drv->refs[vcc_idx] & ADC_NG_REF_MCU_VCC)) {
        if (!(drv->refs[++vcc_idx])) {
            return -ENOTSUP;
        }
    }

    return adc_ng_measure_ref(adc, vcc_idx, dest_mv);
}

int adc_ng_ntc(uint8_t adc, uint8_t chan, const adc_ng_ntc_t *ntc, int16_t *temp)
{
    assert(adc < ADC_NG_NUMOF);
    assert(ntc && temp);

    uint8_t res = adc_ng_max_res(adc);
    uint16_t ref_mv = ntc->offset << 1;
    int retval = adc_ng_init(adc, chan, res, &ref_mv);
    if (retval) {
        return retval;
    }

    uint16_t vin;
    retval = adc_ng_voltage(adc, &vin);
    adc_ng_off(adc);
    if (retval) {
        return retval;
    }

    uint32_t tmp = vin;
    tmp -= ntc->offset;
    tmp *= (uint32_t)ntc->coefficient;
    tmp >>= 10;
    *temp = (int16_t)tmp;

    return 0;
}

int adc_ng_entropy(uint8_t adc, void *_dest, size_t size)
{
    uint8_t *dest = _dest;
    assert(adc < ADC_NG_NUMOF);
    assert(dest && size);
    const adc_ng_driver_t *drv = adc_ng_drivers[adc];

    if (!drv->entropy_bits) {
        return -ENOTSUP;
    }

    uint8_t *end = dest + size;
    void *handle = adc_ng_handles[adc];

    uint8_t res = adc_ng_max_res(adc);
    uint16_t ref_mv = 0;
    int retval = adc_ng_init(adc, ADC_NG_CHAN_ENTROPY, res, &ref_mv);
    if (retval) {
        return retval;
    }

    uint16_t entropy = 0;
    unsigned entropy_bits = 0;
    const unsigned bytes_per_iteration = drv->entropy_bits / 8;
    const unsigned bits_per_iteration = drv->entropy_bits % 8;
    /* Bitmask to select only the bits_per_iteration least significant bits */
    const uint8_t entropy_mask = (1 << bits_per_iteration) - 1;
    while (dest < end) {
        union {
            uint32_t u32;
            uint8_t u8[4];
        } tmp;
        retval = drv->single(handle, &tmp.u32);
        if (retval) {
            drv->off(handle);
            return retval;
        }

        for (unsigned i = 0; i < bytes_per_iteration; i++) {
            *dest++ = tmp.u8[i];
            if (dest >= end) {
                drv->off(handle);
                return 0;
            }
        }
        entropy <<= bits_per_iteration;
        entropy |= tmp.u8[0] & entropy_mask;
        entropy_bits += bits_per_iteration;
        if (entropy_bits >= 8) {
            *dest++ = (uint8_t)entropy;
            entropy >>= 8;
            entropy_bits -= 8;
        }
    }

    drv->off(handle);
    return 0;
}
