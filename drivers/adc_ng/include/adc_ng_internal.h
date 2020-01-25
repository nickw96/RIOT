/*
 * Copyright (C) 2020 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     drivers_adc
 * @{
 *
 * @file
 * @brief       Internal types used in the common ADC API
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef ADC_NG_INTERNAL_H
#define ADC_NG_INTERNAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(ADC_NG_NUMOF) || defined(DOXYGEN)
/**
 * @brief   Number of ADC devices supported
 *
 * If a board supports more than one ADC, it has to define `ADC_NG_NUMOF` in
 * `perpih_conf.h` and implement (but not declare)
 * `adc_ng_driver_t *adc_ng_drivers[ADC_NG_NUMOF]` and
 * `void *adc_ng_handles[ADC_NG_NUMOF]` (e.g. in `board.c`).
 */
#define ADC_NG_NUMOF                    (1U)
#endif

/**
 * @brief   This special channel must refer to an internal fixed reference
 *          voltage used as input
 *
 * ADCs not supporting this will return `-ERANGE` when this channel is
 * selected. If this is supported, it can be used to measure the correct value
 * of voltage references depending on supply voltages (including the MCUs
 * supply voltage, if selectable as reference voltage). This allows to
 * compensate for differences between nominal and actual voltage reference
 * during conversion to physical units.
 *
 * In case of boards running directly of a battery, measuring the supply
 * voltage is particularly useful to estimate the remaining battery charge.
 */
#define ADC_NG_CHAN_FIXED_REF           (UINT8_MAX)

/**
 * @brief   This special channel must refer to an internally connected
 *          thermistor
 */
#define ADC_NG_CHAN_NTC                 (UINT8_MAX - 1)

/**
 * @brief   This special channel must refer to a channel collecting entropy
 *
 * @note    When this channel is selected, a driver can (and likely should)
 *          ignore the requested resolution and reference voltage.
 *
 * When this channel is used, the @ref adc_ng_driver_t::entropy_bits least
 * significant of every sample obtained will contain a some (possibly weak)
 * entropy. The contents of the remaining bits are undefined.
 */
#define ADC_NG_CHAN_ENTROPY             (UINT8_MAX - 2)

/**
 * @brief   Use this value in @ref adc_ng_driver_t::fixed_ref_input to
 *          indicate that no fixed reference can be used as input
 */
#define ADC_NG_NO_FIXED_INPUT           (UINT8_MAX)

/**
 * @brief   Flag to indicate the MCUs supply voltage is used as reference
 *
 * When a known, lower reference voltage can be selected as input and is
 * sampled using the MCU's VCC as reference, the MCU's VCC can be deduced.
 */
#define ADC_NG_REF_MCU_VCC              (0x8000U)

/**
 * @brief   Flag a reference voltage as calibrated
 *
 * E.g. the 1.1V bandgap voltage reference available on many ATmega has
 * production variations of +-100mV, but is extremely stable; so it will
 * consistently create the same voltage for a wide range of supply voltage and
 * operating temperatures. So if the actual value of the reference is e.g.
 * written to EEPROM and restored on boot, the reference value should be marked
 * as calibrated.
 */
#define ADC_NG_REF_CALIBRATED           (0x4000U)
/**
 * @brief   Access the voltage value without flags
 */
#define ADC_NG_REF_MASK                 (0x3fffU)

/**
 * @brief   Description of an thermistor to use for temperature measurements
 */
typedef struct {
    /**
     * @brief   Contains the temperature coefficient of the NTC, or zero
     *
     * The coefficient is is given in 1/1024 mV per 0.1 °C. The resulting
     * temperature in 0.1 °C is calculated from the measured voltage using:
     *
     *    T[d°C] = (coefficient * (mV - offset)) / 1024
     */
    uint16_t coefficient;
    /**
     * @brief   The offset in mV to use for obtaining the temperature
     *
     * See @ref adc_ng_ntc_t::coefficient
     */
    uint16_t offset;
} adc_ng_ntc_t;

/**
 * @brief   Internal driver interface
 */
typedef struct {
    /**
     * @brief   Initialize the given ADC channel and prepare it for sampling
     * @param           handle  Handle of the ADC
     * @param[in]       chan    The ADC channel to initialize
     * @param[in]       res     The resolution to select
     * @param[in        ref     Index of the reference to use (@ref adc_ng_driver_t::refs)
     *
     * @retval  0               Success
     * @retval  -ENXIO          Invalid channel given in @p channel
     * @retval  -EALREADY       The ADC is already powered and configured
     * @retval  <0              Other error (see device driver doc)
     *
     * @post    When 0 is returned, the channel @p channel is ready take samples
     * @post    If @p res contains an unsupported resolution, an assert blows up
     * @post    If `-EALREADY` is returned, the ADC is state remains unchanged
     * @post    On other error codes the ADC is powered down
     *
     * @note    A call to @ref adc_ng_driver_t::off is needed to disable the channel
     *          again and preserve power
     */
    int (*init)(void *handle, uint8_t chan, uint8_t res, uint8_t ref);
    /**
     * @brief   Disable the given ADC channel again and bring the ADC into a low
     *          power state, unless other ADC channels are still onchannel
     *
     * @param           handle  Handle of the ADC
     */
    void (*off)(void *handle);
    /**
     * @brief   Runs a single conversion and returns the sample
     *
     * @param           handle  Handle of the ADC
     * @param[out]      dest    The sample will be stored here
     *
     * @return  The result of the conversion
     *
     * @pre     The ADC has been initialized, see @ref adc_ng_driver_t::init
     *
     * @post    If the ADC is not initialized, an assertion blows up
     *
     * @retval  0           Success
     * @retval  <0          Error (check device driver's doc for error codes)
     */
    int (*single)(void *handle, uint32_t *dest);
#ifdef MODULE_ADC_BURST
    /**
     * @brief   Runs a burst conversion acquiring multiple samples in fast
     *          succession
     *
     * @param           handle  Handle of the ADC
     * @param[out]      dest    Buffer to write the results of the burst read to
     * @param[in]       num     Number of samples to burst read
     *
     * @pre     The channel given in @p channel is currently initialized, see
     *          @ref adc_ng_driver_t::init
     *
     * @post    If the ADC is not initialized, an assertion blows up
     *
     * @retval  0           Success
     * @retval  <0          Error (check device driver's doc for error codes)
     */
    int (*burst)(void *handle, uint32_t *dest, size_t num);
#endif /* MODULE_ADC_BURST */
    /**
     * @brief   Bitmap containing the supported ADC resolutions
     *
     * If e.g. the resolutions 4bit, 6bit and 8bit are supported it should have
     * the value `BIT4 | BIT6 | BIT8`. Thus, currently the highest resolution
     * supported is 31 bit.
     */
    uint32_t res_supported;
    /**
     * @brief   The reference voltages supported in ascending order
     *
     * This list should be sorted in ascending order and terminated with a
     * value of zero. The reference voltage are a bitmask with the 14 least
     * significant bits containing the voltage value in mV, and the two
     * most significant bits indicate whether the reference voltage is
     * calibrated (@ref ADC_NG_REF_CALIBRATED) and if the MCUs supply voltage
     * is used as reference (@ref ADC_NG_REF_MCU_VCC).
     */
    const uint16_t *refs;
    /**
     * @brief   Parameters of the internally connected thermistor or `NULL`
     */
    const adc_ng_ntc_t *ntc;
    /**
     * @brief   The index of the reference voltage that can be used as input
     *          using channel @ref ADC_NG_CHAN_FIXED_REF
     *
     * In case the fixed reference voltage can only be used as input, and not
     * set up as reference during conversion, add this reference to @ref
     * adc_ng_driver_t::refs after the last item (identified using value zero)
     * and use that index here. This will prevent that reference to be used
     * except as input.
     *
     * Use the special value @ref ADC_NG_NO_FIXED_INPUT if no fixed reference
     * voltage can be used as input.
     */
    uint8_t fixed_ref_input;
    /**
     * @brief   The number of least significant bits containing entropy
     *
     * @note    This only refers to channel @ref ADC_NG_CHAN_ENTROPY
     *
     * A value of zero must be used when the ADC does not support harvesting
     * entropy. Note that even a source of weak entropy can be useful, when
     * fed into a entropy extractor. Often only taking only the least
     * significant bit when measuring a noisy source is a good choice
     * (so a value of `1`).
     */
    uint8_t entropy_bits;
} adc_ng_driver_t;

/**
 * @brief   Array containing pointer to the ADC drivers to used
 */
extern const adc_ng_driver_t * const adc_ng_drivers[ADC_NG_NUMOF];
/**
 * @brief   Array containing pointers to the handles the ADC drivers work on
 */
extern void * const adc_ng_handles[ADC_NG_NUMOF];
/**
 * @brief   Currently selected reference voltage in mV
 */
extern uint16_t adc_ng_refs[ADC_NG_NUMOF];
/**
 * @brief   Currently selected resolution
 */
extern uint8_t adc_ng_res[ADC_NG_NUMOF];

/**
 * @brief   Convert an ADC sample to a voltage level in mV
 *
 * @param[in]       adc         ADC that was used to take the sample
 * @param[in]       sample      The ADC sample to convert
 *
 * @return  The voltage level in mV
 *
 * @pre     The ADC identified by @p adc has not been re-initialized since
 *          taking the sample. (But it can be offline.)
 */
uint16_t adc_ng_convert(uint8_t adc, uint32_t sample);

/**
 * @brief   Measure the actual value of a reference voltage by selecting
 *          the constant voltage reference as input
 *
 * @param[in]       adc         ADC of which the reference voltage should be
 *                              measured
 * @param[in]       ref_idx     Index of the reference to measure
 * @param[out]      dest_mv     Measured value of the voltage reference in mV
 */
int adc_ng_measure_ref(uint8_t adc, uint8_t ref_idx, uint16_t *dest_mv);

#ifdef __cplusplus
}
#endif

#endif /* ADC_NG_INTERNAL_H */
/** @} */
