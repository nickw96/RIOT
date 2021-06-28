/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup         cpu_rp2040
 * @{
 *
 * @file
 * @brief           RP2040 specific definitions for handling peripherals
 *
 * @author          Fabian Hüßler <fabian.huessler@ovgu.de>
 * @author          Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef PERIPH_CPU_H
#define PERIPH_CPU_H

#include "cpu.h"
#include "vendor/RP2040.h"
#include "reg_atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   System core clock speed is fixed to 125MHz
 */
#define CLOCK_CORECLOCK     (125000000U)

/**
 * @brief   Clock for UART0 and UART1 peripherals
 */
#define CLOCK_CLKPERI       CLOCK_CORECLOCK

/**
 * @brief   Periphery blocks that can be reset
 */
#define RESETS_RESET_MASK               \
    (RESETS_RESET_usbctrl_Msk       |   \
     RESETS_RESET_uart1_Msk         |   \
     RESETS_RESET_uart0_Msk         |   \
     RESETS_RESET_timer_Msk         |   \
     RESETS_RESET_tbman_Msk         |   \
     RESETS_RESET_sysinfo_Msk       |   \
     RESETS_RESET_syscfg_Msk        |   \
     RESETS_RESET_spi1_Msk          |   \
     RESETS_RESET_spi0_Msk          |   \
     RESETS_RESET_rtc_Msk           |   \
     RESETS_RESET_pwm_Msk           |   \
     RESETS_RESET_pll_usb_Msk       |   \
     RESETS_RESET_pll_sys_Msk       |   \
     RESETS_RESET_pio1_Msk          |   \
     RESETS_RESET_pio0_Msk          |   \
     RESETS_RESET_pads_qspi_Msk     |   \
     RESETS_RESET_pads_bank0_Msk    |   \
     RESETS_RESET_jtag_Msk          |   \
     RESETS_RESET_io_qspi_Msk       |   \
     RESETS_RESET_io_bank0_Msk      |   \
     RESETS_RESET_i2c1_Msk          |   \
     RESETS_RESET_i2c0_Msk          |   \
     RESETS_RESET_dma_Msk           |   \
     RESETS_RESET_busctrl_Msk       |   \
     RESETS_RESET_adc_Msk)

#define GPIO_PIN(port, pin)     (pin)

/**
 * @brief   Possible drive strength values for @ref gpio_pad_ctrl_t::driver_strength
 */
enum {
    DRIVE_STRENGTH_2MA,         /**< set driver strength to 2 mA */
    DRIVE_STRENGTH_4MA,         /**< set driver strength to 4 mA */
    DRIVE_STRENGTH_8MA,         /**< set driver strength to 8 mA */
    DRIVE_STRENGTH_12MA,        /**< set driver strength to 12 mA */
    DRIVE_STRENGTH_NUMOF        /**< number of different drive strength options */
};

/**
 * @brief   Memory layout of GPIO control register in pads bank 0
 */
typedef struct {
    uint32_t slew_rate_fast         : 1;    /**< set slew rate control to fast */
    uint32_t schmitt_trig_enable    : 1;    /**< enable Schmitt trigger */
    uint32_t pull_down_enable       : 1;    /**< enable pull down resistor */
    uint32_t pull_up_enable         : 1;    /**< enable pull up resistor */
    uint32_t drive_strength         : 2;    /**< GPIO driver strength */
    uint32_t input_enable           : 1;    /**< enable as input */
    uint32_t output_disable         : 1;    /**< disable output, overwrite output enable from peripherals */
    uint32_t                        : 24;
} gpio_pad_ctrl_t;

/**
 * @brief   Possible function values for @ref gpio_io_ctrl_t::function_select
 */
enum {
    FUNCTION_SELECT_SPI     = 1,    /**< connect pin to the SPI peripheral
                                     *   (MISO/MOSI/SCK depends on pin) */
    FUNCTION_SELECT_UART    = 2,    /**< connect pin to the UART peripheral
                                     *   (TXD/RXD depends on pin) */
    FUNCTION_SELECT_I2C     = 3,    /**< connect pin to the I2C peripheral
                                     *   (SCL/SDA depends on pin) */
    FUNCTION_SELECT_PWM     = 4,    /**< connect pin to the timer for PWM
                                     *   (channel depends on pin) */
    FUNCTION_SELECT_SIO     = 5,    /**< use pin as vanilla GPIO */
    FUNCTION_SELECT_PIO0    = 6,    /**< connect pin to the first PIO peripheral */
    FUNCTION_SELECT_PIO1    = 7,    /**< connect pin to the second PIO peripheral */
    FUNCTION_SELECT_CLOCK   = 8,    /**< connect pin to the timer (depending on pin: external clock,
                                     *   clock output, or not supported) */
    FUNCTION_SELECT_USB     = 9,    /**< connect pin to the USB peripheral
                                     *   (function depends on pin) */
};

/**
 * @brief   Possible function values for @ref gpio_io_ctrl_t::output_override
 */
enum {
    OUTPUT_OVERRIDE_NORMAL,         /**< drive pin from connected peripheral */
    OUTPUT_OVERRIDE_INVERT,         /**< drive pin from connected peripheral, but invert output */
    OUTPUT_OVERRIDE_LOW,            /**< drive pin low, overriding peripheral signal */
    OUTPUT_OVERRIDE_HIGH,           /**< drive pin high, overriding peripheral signal */
    OUTPUT_OVERRIDE_NUMOF           /**< number of possible output override settings */
};

/**
 * @brief   Possible function values for @ref gpio_io_ctrl_t::output_enable_override
 */
enum {
    OUTPUT_ENABLE_OVERRIDE_NOMARL,  /**< enable output as specified by connected peripheral */
    OUTPUT_ENABLE_OVERRIDE_INVERT,  /**< invert output enable setting of peripheral */
    OUTPUT_ENABLE_OVERRIDE_DISABLE, /**< disable output, overriding peripheral signal */
    OUTPUT_ENABLE_OVERRIDE_ENABLE,  /**< enable output, overriding peripheral signal */
    OUTPUT_ENABLE_OVERRIDE_NUMOF    /**< number of possible output enable override settings */
};

/**
 * @brief   Possible function values for @ref gpio_io_ctrl_t::input_override
 */
enum {
    INPUT_OVERRIDE_NOMARL,          /**< don't mess with peripheral input signal */
    INPUT_OVERRIDE_INVERT,          /**< invert signal to connected peripheral */
    INPUT_OVERRIDE_LOW,             /**< signal low to connected peripheral */
    INPUT_OVERRIDE_HIGH,            /**< signal high to connected peripheral */
    INPUT_OVERRIDE_NUMOF            /**< number of possible input override settings */
};

/**
 * @brief   Possible function values for @ref gpio_io_ctrl_t::irqw_override
 */
enum {
    IRQ_OVERRIDE_NORMAL,            /**< don't mess with IRQ signal */
    IRQ_OVERRIDE_INVERT,            /**< invert IRQ signal */
    IRQ_OVERRIDE_LOW,               /**< set IRQ signal to low */
    IRQ_OVERRIDE_HIGH,              /**< set IRQ signal to high */
    IRQ_OVERRIDE_NUMOF              /**< number of possible IRQ override settings */
};

/**
 * @brief   Overwrite the default gpio_t type definition
 * @{
 */
#define HAVE_GPIO_T
typedef uint32_t gpio_t;
/** @} */

/**
 * @brief   Definition of a fitting UNDEF value
 */
#define GPIO_UNDEF          UINT32_MAX

/**
 * @brief   Override GPIO active flank values
 * @{
 */
#define HAVE_GPIO_FLANK_T
typedef enum {
    GPIO_LEVEL_LOW  = 0x1,          /**< emit interrupt level-triggered on low input */
    GPIO_LEVEL_HIGH = 0x2,          /**< emit interrupt level-triggered on low input */
    GPIO_FALLING    = 0x4,          /**< emit interrupt on falling flank */
    GPIO_RISING     = 0x8,          /**< emit interrupt on rising flank */
    GPIO_BOTH       = 0xc           /**< emit interrupt on both flanks */
} gpio_flank_t;
/** @} */

/**
 * @brief   Memory layout of GPIO control register in IO bank 0
 */
typedef struct {
    uint32_t function_select        : 5;    /**< select GPIO function */
    uint32_t                        : 3;
    uint32_t output_overide         : 2;    /**< output value override */
    uint32_t                        : 2;
    uint32_t output_enable_overide  : 2;    /**< output enable override */
    uint32_t                        : 2;
    uint32_t input_override         : 2;    /**< input value override */
    uint32_t                        : 10;
    uint32_t irq_override           : 2;    /**< interrupt inversion override */
    uint32_t                        : 2;
} gpio_io_ctrl_t;

typedef struct {
    UART0_Type *dev;
    gpio_t rx_pin;
    gpio_t tx_pin;
    IRQn_Type irqn;
} uart_conf_t;

/**
 * @brief   Get the PAD control register for the given GPIO pin as word
 */
static inline volatile uint32_t * gpio_pad_register_u32(uint8_t pin)
{
    return (uint32_t *)(PADS_BANK0_BASE + 4 + (pin << 2));
}

/**
 * @brief   Get the PAD control register for the given GPIO pin as struct
 */
static inline volatile gpio_pad_ctrl_t * gpio_pad_register(uint8_t pin)
{
    return (gpio_pad_ctrl_t *)gpio_pad_register_u32(pin);
}

/**
 * @brief   Get the IO control register for the given GPIO pin as word
 */
static inline volatile uint32_t * gpio_io_register_u32(uint8_t pin)
{
    return (uint32_t *)(IO_BANK0_BASE + 4 + (pin << 3));
}

/**
 * @brief   Get the IO control register for the given GPIO pin as struct
 */
static inline volatile gpio_io_ctrl_t * gpio_io_register(uint8_t pin)
{
    return (gpio_io_ctrl_t *)gpio_io_register_u32(pin);
}

/**
 * @brief   Reset hardware components
 *
 * @param   components bitmask of components to be reset,
 *          @see RESETS_RESET_MASK
 */
static inline void periph_reset(uint32_t components)
{
    reg_atomic_set(&RESETS->RESET.reg, components);
}
/**
 * @brief   Waits until hardware components have been reset
 *
 * @param   components bitmask of components that must have reset,
 *          @see RESETS_RESET_MASK
 */
static inline void periph_reset_done(uint32_t components)
{
    reg_atomic_clear(&RESETS->RESET.reg, components);
    while ((~RESETS->RESET_DONE.reg) & components) { }
}

/**
 * @name    RP2040 clock configuration
 * @{
 */

/**
 * @brief   Configure the system clock to run from the reference clock,
 *          which is the default on boot
 *
 * @param   f_in        Input frequency of the reference clock
 * @param   f_out       Output frequency of the system clock
 * @param   source      Clock source
 */
void clock_sys_configure_source(uint32_t f_in, uint32_t f_out,
                                CLOCKS_CLK_SYS_CTRL_SRC_Enum source);

/**
 * @brief   Configure the system clock to run from an auxiliary clock source,
 *          like PLL
 *
 * @note    The auxiliary must have been configured beforehand
 *
 * @param   f_in        Input frequency of the auxiliary clock source
 * @param   f_out       Output frequency of the system clock
 * @param   aux         Which auxiliary clock source to use
 */
void clock_sys_configure_aux_source(uint32_t f_in, uint32_t f_out,
                                    CLOCKS_CLK_SYS_CTRL_AUXSRC_Enum aux);

/**
 * @brief   Configure the reference clock to run from a clock source,
 *          which is either the ROSC or the XOSC
 *
 * @note    Make sure that ROSC or XOSC are properly set up
 *
 * @param   f_in        Input frequency of the reference clock
 * @param   f_out       Output frequency of the system clock
 * @param   source      Clock source
 */
void clock_ref_configure_source(uint32_t f_in, uint32_t f_out,
                                CLOCKS_CLK_REF_CTRL_SRC_Enum source);

/**
 * @brief   Configure the reference clock to run from an auxiliary clock source,
 *          like PLL
 *
 * @note    The auxiliary must have been configured beforehand
 *
 * @param   f_in        Input frequency of the auxiliary clock source
 * @param   f_out       Output frequency of the reference clock
 * @param   aux         Which auxiliary clock source to use
 */
void clock_ref_configure_aux_source(uint32_t f_in, uint32_t f_out,
                                    CLOCKS_CLK_REF_CTRL_AUXSRC_Enum aux);

/**
 * @brief   Configure the peripheral clock to run from a dedicated auxiliary
 *          clock source
 *
 * @param   aux     Auxiliary clock source
 */
void clock_periph_configure(CLOCKS_CLK_PERI_CTRL_AUXSRC_Enum aux);

/**
 * @brief   Configure gpio21 as clock output pin
 *
 * @details Can be used as an external clock source for another circuit or
 *          to check the expected signal with a logic analyzer
 *
 * @param   f_in        Input frequency
 * @param   f_out       Output frequency
 * @param   aux         Auxiliary clock source
 */
void clock_gpout0_configure(uint32_t f_in, uint32_t f_out, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_Enum aux);

/**
 * @brief   Configure gpio23 as clock output pin
 *
 * @details Can be used as an external clock source for another circuit or
 *          to check the expected signal with a logic analyzer
 *
 * @param   f_in        Input frequency
 * @param   f_out       Output frequency
 * @param   aux         Auxiliary clock source
 */
void clock_gpout1_configure(uint32_t f_in, uint32_t f_out, CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_Enum aux);

/**
 * @brief   Configure gpio24 as clock output pin
 *
 * @details Can be used as an external clock source for another circuit or
 *          to check the expected signal with a logic analyzer
 *
 * @param   f_in        Input frequency
 * @param   f_out       Output frequency
 * @param   aux         Auxiliary clock source
 */
void clock_gpout2_configure(uint32_t f_in, uint32_t f_out, CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_Enum aux);

/**
 * @brief   Configure gpio25 as clock output pin
 *
 * @details Can be used as an external clock source for another circuit or
 *          to check the expected signal with a logic analyzer
 *
 * @param   f_in        Input frequency
 * @param   f_out       Output frequency
 * @param   aux         Auxiliary clock source
 */
void clock_gpout3_configure(uint32_t f_in, uint32_t f_out, CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_Enum aux);

/** @} */

/**
 * @name    RP2040 PLL configuration
 * @{
 */

/**
 * @brief   Start the PLL for the system clock
 *          output[MHz] = @p f_ref / @p ref_div * @p vco_feedback_scale / @p post_div_1 * @p post_div_2
 *
 * @note    Usual setting should be (12 MHz, 1, 125, 6, 2) to get a 125 MHz system clock signal
 *
 * @param   f_ref       Input clock frequency from the XOSC
 * @param   ref_div     Input clock divisor
 * @param   vco_feedback_scale  VCO feedback scales
 * @param   post_div_1  Output post divider factor 1
 * @param   post_div_2  Output post divider factor 2
 */
void pll_start_sys(uint32_t f_ref, uint8_t ref_div,
                   uint16_t vco_feedback_scale,
                   uint8_t post_div_1, uint8_t post_div_2);

/**
 * @brief   Start the PLL for the system clock
 *          output[MHz] = @p f_ref / @p ref_div * @p vco_feedback_scale / @p post_div_1 * @p post_div_2
 *
 * @note    Usual setting should be (12 MHz, 1, 40, 5, 2) to get a 48 MHz USB clock signal
 *
 * @param   f_ref       Input clock frequency from the XOSC
 * @param   ref_div     Input clock divisor
 * @param   vco_feedback_scale  VCO feedback scales
 * @param   post_div_1  Output post divider factor 1
 * @param   post_div_2  Output post divider factor 2
 */
void pll_start_usb(uint32_t f_ref, uint8_t ref_div,
                   uint16_t vco_feedback_scale,
                   uint8_t post_div_1, uint8_t post_div_2);

/**
 * @brief   Stop the PLL of the system clock
 */
void pll_stop_sys(void);

/**
 * @brief   Stop the PLL of the USB clock
 */
void pll_stop_usb(void);

/**
 * @brief   Reset the PLL of the system clock
 */
void pll_reset_sys(void);

/**
 * @brief   Reset the PLL of the USB clock
 */
void pll_reset_usb(void);

/** @} */

/**
 * @name    RP2040 XOSC configuration
 * @{
 */
/**
 * @brief   Configues the Crstal to run.
 *          Should be configured to 12 MHz which is the default as
 *          described in the hardware manual.
 *          The minimum is 1 MHz and the maximum is 15 MHz.
 *
 * @param   f_ref       Desired frequency
 */
void xosc_start(uint32_t f_ref);

/**
 * @brief   Stop the crystal.
 */
void xosc_stop(void);

/** @} */

/**
 * @name    RP2040 ROSC configuration
 * @{
 */

/**
 * @brief   Start the ring oscillator in default mode.
 *          The ROSC is running at boot time but may be turned off
 *          to save power when switching to the accurate XOSC.
 *          The default ROSC provides an instable frequency of 1.8 MHz to 12 MHz.
 */
void rosc_start(void);

/**
 * @brief   Turn off the ROSC to save power.
 *          The system clock must be switched to to another lock source
 *          before the ROSC is stopped, other wise the chip will be lock up.
 */
void rosc_stop(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CPU_H */
/** @} */
