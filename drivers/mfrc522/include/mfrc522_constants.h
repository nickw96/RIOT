/*
 * Copyright (C) 2019 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_mfrc522
 *
 * @{
 * @file
 * @brief       Constants and magic numbers used in the MFRC522 driver
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef MFRC522_CONSTANTS_H
#define MFRC522_CONSTANTS_H

#include "bitarithm.h"
#include "board.h"
#include "mfrc522.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   The MFRC522 uses SPI mode 0
 */
#define MFRC522_SPI_MODE                SPI_MODE_0

/**
 * @brief   Time to pull reset pin low in microseconds to trigger a reset
 *
 * The MFRC522 reset pin is filtered by an input hysteresis and must be pulled
 * low for at least 100ns to successfully trigger a reset. xtimer can not sleep
 * for less than one microsecond, so we go for 1µs.
 */
#define MFRC522_RESET_TIME_US           (100U)

/**
 * @brief   Time the MFRC522 needs to start in milliseconds
 *
 * The startup time is the sum of the time it takes for the crystal oscillator
 * to start and the internal boot up. The internal boot up takes 37.74µs
 */
#define MFRC522_STARTUP_TIME_MS         (1U)

/**
 * @name    Access modifiers for registers access
 *
 * Binary-or these masks into the address
 * @{
 */
#define MFRC522_READ                    (0x80)  /**< Read from a register */
#define MFRC522_WRITE                   (0x00)  /**< Write into a register */
/** @} */ /* End of access modifiers */

/**
 * @name    Registers addresses of the MFRC522 RFID reader
 * @{
 */
/**
 * @name    Command and status registers
 * @{
 */
#define MFRC522_REG_CMD                 (0x01)  /**< Write here to start/stop cmds */
#define MFRC522_REG_IRQ_EN_COM          (0x02)  /**< Configure IRQs (1/2) */
#define MFRC522_REG_IRQ_EN_DIV          (0x03)  /**< Configure IRQs (2/2) */
#define MFRC522_REG_IRQ_COM             (0x04)  /**< IRQ bits (1/2) */
#define MFRC522_REG_IRQ_DIV             (0x05)  /**< IRQ bits (2/2) */
#define MFRC522_REG_ERROR_MASK          (0x06)  /**< Error information on last cmd */
#define MFRC522_REG_STATUS_1            (0x07)  /**< First status mask */
#define MFRC522_REG_STATUS_2            (0x08)  /**< Second status mask */
#define MFRC522_REG_FIFO                (0x09)  /**< Read/write one FIFO-byte */
#define MFRC522_REG_FIFO_FILL           (0x0a)  /**< Get number of bytes in FIFO */
#define MFRC522_REG_FIFO_THRD           (0x0c)  /**< IRQ when free bytes in FIFO reach or drop below this threshold */
#define MFRC522_REG_BIT_FRAMING         (0x0d)  /**< Bit framing and flag for transmission for TX_RX cmd */
/** @} */ /* End of command and status registers */
/**
 * @name    Configuration registers
 * @{
 */
#define MFRC522_REG_MODE                (0x11)  /**< Mode register (ModeReg in datasheet) */
#define MFRC522_REG_TX_CONTROL          (0x14)  /**< TX control register (TxControlReg in datasheet) */
#define MFRC522_REG_FORCE_ASK           (0x15)  /**< Write 0x40 to force ASK modulation (TxASKReg in datasheet) */
#define MFRC522_REG_CRC_MSB             (0x21)  /**< High byte of the CRC calculation */
#define MFRC522_REG_CRC_LSB             (0x22)  /**< Low byte of the CRC calculation */
#define MFRC522_REG_TIMER_MODE          (0x2a)  /**< Timer mode (TModeReg in datasheet) + 4 bits of timer prescaler */
#define MFRC522_REG_TIMER_PRESCALER     (0x2b)  /**< Timer prescaler (TPrescalerReg in datasheet) */
#define MFRC522_REG_TIMER_RELOAD_MSB    (0x2c)  /**< MSB of 16 bit timer reload value (TRealoadReg in datasheet) */
#define MFRC522_REG_TIMER_RELOAD_LSB    (0x2d)  /**< LSB of 16 bit timer reload value (TRealoadReg in datasheet) */
/** @} */ /* End of configuration registers */
/**
 * @name    Test registers
 * @{
 */
#define MFRC522_REG_VERSION             (0x37)  /**< Device version */
/** @} */ /* End of test registers */
/** @} */ /* End of register addresses */

/**
 * @name    Commands supported by the MFRC522
 * @{
 */
#define MFRC522_CMD_IDLE                (0x0)   /**< Go back to IDLE, cancel current CMD */
/**
 * @brief   Transfer 25 FIFO bytes to internal memory or back
 *
 * If the FIFO contains data, 25 bytes of the FIFO are backed up in internal
 * memory that persists resets (even via RST-pin), but not power losses. The
 * command @ref MFRC522_CMD_RND_ID overwrites the first 10 bytes of backed up
 * FIFO data when executed.
 *
 * If the FIFO is empty, the (previously) backed up 25 bytes (or the random ID
 * generated with @ref MFRC522_CMD_RND_ID) are placed back into the FIFO.
 */
#define MFRC522_CMD_MEM                 (0x1)
/**
 * @brief   Generate a 10 byte random ID and stores it in the internal memory
 *
 * Run @ref MFRC522_CMD_MEM when the FIFO is idle to retrieve the random ID
 * (and the content of the next 15 bytes of the internal memory).
 */
#define MFRC522_CMD_RND_ID              (0x2)
#define MFRC522_CMD_CRC                 (0x3)   /**< Feed the FIFO to the CRC coprocessor */
#define MFRC522_CMD_TX                  (0x4)   /**< Transmit FIFO contents */
/**
 * @brief   Used to update @ref MFRC522_REG_CMD
 *
 * When bits in @ref MFRC522_REG_CMD need to be changed without
 * aborting/changing the currently executed command, this NOOP can be used as
 * command.
 */
#define MFRC522_CMD_NOOP                (0x7)
#define MFRC522_CMD_RX                  (0x8)   /**< Receive into FIFO */
/**
 * @brief   Transmit FIFO contents then receive into FIFO
 *
 * @warning Waits for @ref MFRC522_REG_BIT_FRAMING_START to be set before it
 *          actually starts to transmit.
 */
#define MFRC522_CMD_TX_RX               (0xc)
#define MFRC522_CMD_AUTH                (0xe)   /**< Start authentication */
#define MFRC522_CMD_RST                 (0xf)   /**< Do a soft reset */
/** @} */ /* End of commands */

/**
 * @name    MFRC522 Version IDs
 * @{
 */
#define MFRC522_VERSION_1               (0x91)  /**< ID of MFRC522 Version 1.0 */
#define MFRC522_VERSION_2               (0x92)  /**< ID of MFRC522 Version 2.0 */
/** @} */

/**
 * @name    MFRC522 Control Bits in @ref MFRC522_REG_IRQ_EN_COM Register
 * @{
 */
#define MFRC522_IRQ_EN_COM_INVERT       BIT7    /**< If set, output of IRQ pin is inverted (low on IRQ) */
#define MFRC522_IRQ_EN_COM_TX           BIT6    /**< Enable IRQ on TX completed */
#define MFRC522_IRQ_EN_COM_RX           BIT5    /**< Enable IRQ on RX completed */
#define MFRC522_IRQ_EN_COM_IDLE         BIT4    /**< Enable IRQ on command completion */
#define MFRC522_IRQ_EN_COM_FIFO_HIGH    BIT3    /**< Enable IRQ when FIFO is (almost) full */
#define MFRC522_IRQ_EN_COM_FIFO_LOW     BIT2    /**< Enable IRQ when FIFO is (almost) empty */
#define MFRC522_IRQ_EN_COM_ERROR        BIT1    /**< Enable IRQ on error */
#define MFRC522_IRQ_EN_COM_TIMER        BIT0    /**< Enable IRQ for timer */
/** @} */

/**
 * @name    MFRC522 Control Bits in @ref MFRC522_REG_IRQ_EN_DIV Register
 * @{
 */
#define MFRC522_IRQ_EN_DIV_PUSH_PULL    BIT7    /**< Use push-pull (1) or open drain (0, default) for IRQ pin */
#define MFRC522_IRQ_EN_DIV_MFIN         BIT4    /**< Enable IRQ on modulation signal from MFIN */
#define MFRC522_IRQ_EN_DIV_CRC          BIT2    /**< Enable IRQ on completion of CRC calculation */
/** @} */

/**
 * @name    MFRC522 Bits in @ref MFRC522_REG_IRQ_COM
 * @{
 */
#define MFRC522_IRQ_COM_SET             BIT7    /**< When writing, set given IRQ flags instead of clearing them */
#define MFRC522_IRQ_COM_TX              BIT6    /**< TX IRQ flag */
#define MFRC522_IRQ_COM_RX              BIT5    /**< TX IRQ flag */
#define MFRC522_IRQ_COM_IDLE            BIT4    /**< Idle IRQ flag */
#define MFRC522_IRQ_COM_FIFO_HIGH       BIT3    /**< FIFO is (almost) full IRQ flag */
#define MFRC522_IRQ_COM_FIFO_LOW        BIT2    /**< FIFO is (almost) empty IRQ flag */
#define MFRC522_IRQ_COM_ERROR           BIT1    /**< Error IRQ flag */
#define MFRC522_IRQ_COM_TIMER           BIT0    /**< Timer IRQ flag */
#define MFRC522_IRQ_COM_CLEAR           0x7f    /**< Mask to clear all IRQ flags */
/** @} */

/**
 * @name    MFRC522 Status Bits in the First Status Byte
 * @{
 */
#define MFRC522_STATUS_1_CRC_OK         BIT6    /**< Set on CRC OK */
#define MFRC522_STATUS_1_CRC_READY      BIT5    /**< Execution of MFRC522_CMD_CRC completed */
#define MFRC522_STATUS_1_IRQ            BIT4    /**< Set if any non-masked IRQ request is active */
#define MFRC522_STATUS_1_TIMER_RUNNING  BIT3    /**< Set if the MFRC522 timer is running */
#define MFRC522_STATUS_1_FIFO_HIGH      BIT1    /**< The FIFO is (almost) full */
#define MFRC522_STATUS_1_FIFO_LOW       BIT0    /**< The FIFO is (almost) empty */
/** @} */

/**
 * @name    MFRC522 Status Bits in the SECOND Status Byte
 * @{
 */
#define MFRC522_STATUS_2_CRYPTO_ON      BIT3    /**< Set if communication is encrypted */
#define MFRC522_STATUS_2_STATE_MASK     (0x7U)  /**< Bitmask to get the modem state */
/** @} */

/**
 * @brief   MFRC522 Modem States
 *
 * @see     MFRC522_STATUS_2_STATE_MASK
 */
enum {
    MFRC522_STATE_IDLE      = 0x0,      /**< Modem idle */
    MFRC522_STATE_TX_READY  = 0x1,      /**< Waiting for start send bit in the bit framing register to be set */
    MFRC522_STATE_TX_WAIT   = 0x2,      /**< Waiting before transmitting as configured in TxWaitReg */
    MFRC522_STATE_TX        = 0x3,      /**< Transmitting */
    MFRC522_STATE_RX_WAIT   = 0x4,      /**< Waiting before receiving as configured in RxWaitReg */
    MFRC522_STATE_RX_READY  = 0x5,      /**< Ready to receive, waiting for data */
    MFRC522_STATE_RX        = 0x6,      /**< Currently receiving */
};
/** @} */

/**
 * @name    MFRC522 TX Control Register Bits
 * @{
 */
#define MFRC522_REG_TX_CONTROL_INV_TX2_ON   BIT7    /**< Invert output on TX2 when enabled */
#define MFRC522_REG_TX_CONTROL_INV_TX1_ON   BIT6    /**< Invert output on TX1 when enabled */
#define MFRC522_REG_TX_CONTROL_INV_TX2_OFF  BIT5    /**< Invert output on TX2 when disabled */
#define MFRC522_REG_TX_CONTROL_INV_TX1_OFF  BIT4    /**< Invert output on TX1 when disabled */
#define MFRC522_REG_TX_CONTROL_TX2_CARRIER  BIT3    /**< Unmodulated energy carrier on TX2 */
#define MFRC522_REG_TX_CONTROL_TX2_ON       BIT1    /**< Enable modulated energy carrier on TX2 */
#define MFRC522_REG_TX_CONTROL_TX1_ON       BIT0    /**< Enable modulated energy carrier on TX1 */
/** @} */

/**
 * @name    Proximity Integrated Circuit Card (PICC) Commands
 * @{
 */
/**
 * @brief   Request A: Wake up cards in the idle state
 *
 * Useful to poll for cards newly entered the field of the reader
 */
#define PICC_CMD_REQA                       0x26
#define PICC_CMD_RATS                       0xe0    /**< Request for Answer to Select */
#define PICC_CMD_MF_AUTH_KEY_A              0x60    /**< Perform authentication with key A */
#define PICC_CMD_MF_AUTH_KEY_B              0x61    /**< Perform authentication with key B */
#define PICC_CMD_HLTA                       0x50    /**< Halt command, type A. Active card should enter state halt */
#define PICC_CMD_WUPA                       0x52    /**< Wake up, type A. Wakes up cards regardless of state */
#define PICC_CMD_CT                         0x88    /**< Cascade tag used during anti collision */
#define PICC_CMD_SEL_CL1                    0x93    /**< Anti collision select, cascade level 1 */
#define PICC_CMD_SEL_CL2                    0x95    /**< Anti collision select, cascade level 2 */
#define PICC_CMD_SEL_CL3                    0x97    /**< Anti collision select, cascade level 3 */
#define PICC_CMD_MF_READ                    0x30    /**< Read a 16 byte block from authenticated sector */
#define PICC_CMD_MF_WRITE                   0xa0    /**< Writes a 16 byte block to authenticated sector */
#define PICC_CMD_MF_DECREMENT               0xc0    /**< Store the decremented current block in the internal data register */
#define PICC_CMD_MF_INCREMENT               0xc1    /**< Store the incremented current block in the internal data register */
#define PICC_CMD_MF_RESTORE                 0xc2    /**< Copy current block into the internal data register */
#define PICC_CMD_MF_TRANSFER                0xb0    /**< Write internal data register to current block */
/** @} */

/**
 * @name    Flags
 * @{
 */
#define MFRC522_FLAG_EXTIRQ                 0x01    /**< Next event handler needs to attend external IRQ */
#define MFRC522_FLAG_POLLING                0x02    /**< Set when polling for cards */
#define MFRC522_FLAG_BUSY                   0x04    /**< Set when transceiver or buffer in use */
#define MFRC522_FLAG_ERROR                  0x08    /**< Set when an error occurred */
#define MFRC522_FLAG_OVERFLOW               0x10    /**< Set when card's response doesn't fit buffer */
/** @} */

/**
 * @name    Bit framing settings in @ref MFRC522_REG_BIT_FRAMING
 * @{
 */
#define MFRC522_REG_BIT_FRAMING_START       BIT7    /**< Start transmission after MFRC522_CMD_TX_RX command */
#define MFRC522_REG_BIT_FRAMING_ALIGN_0     0       /**< Store received LSB in bit 0*/
#define MFRC522_REG_BIT_FRAMING_ALIGN_1     0x10    /**< Store received LSB in bit 1 */
#define MFRC522_REG_BIT_FRAMING_ALIGN_7     0x70    /**< Store received LSB in bit 7, next bit in following byte */
#define MFRC522_REG_BIT_FRAMING_BITS_7      0x07    /**< Send only 7 bits of the last byte in FIFO (for bit-oriented protocols useful) */
#define MFRC522_REG_BIT_FRAMING_BITS_8      0x00    /**< Send all bits of the last byte in the FIFO */
/** @} */

/**
 * @name    Settings in the @ref MFRC522_REG_TIMER_MODE
 * @{
 */
#define MFRC522_REG_TIMER_MODE_AUTO         BIT7    /**< Automatically start timer at the end of the transmission */
#define MFRC522_REG_TIMER_MODE_NON_GATED    0x00    /**< Run timer in non-gated mode */
#define MFRC522_REG_TIMER_MODE_GATED_MFIN   0x20    /**< Run timer gated by pin MFIN */
#define MFRC522_REG_TIMER_MODE_GATED_AUX1   0x40    /**< Run timer gated by pin AUX1 */
#define MFRC522_REG_TIMER_MODE_AUTO_RESTART 0x10    /**< Automatically restart timer from the reload value instead of firing IRQ */
/** @} */

/**
 * @name    Settings in the @ref MFRC522_REG_FORCE_ASK register
 * @{
 */
#define MFRC522_REG_FORCE_ASK_ENABLED       0x40    /**< Force ASK modulation */
#define MFRC522_REG_FORCE_ASK_DISABLED      0x00    /**< Don't force ASK modulation */
/** @} */

/**
 * @name    Settings in the @ref MFRC522_REG_MODE register
 * @{
 */
#define MFRC522_REG_MODE_MSB_FIRST          BIT7    /**< Calculated CRC with MSB first */
#define MFRC522_REG_MODE_TX_WAIT_RF         BIT5    /**< Wait for RF field being generated before starting TX */
#define MFRC522_REG_MODE_RFU                0x14    /**< Bits 2 and 4 are reserved for future use, but they are set by default */
#define MFRC522_REG_MODE_MFIN_ACTIVE_HIGH   BIT3    /**< Polarity of MFIN is active HIGH */
#define MFRC522_REG_MODE_MFIN_ACTIVE_LOW    0x00    /**< Polarity of MFIN is active LOW */
#define MFRC522_REG_MODE_CRC_PRESET_0000    0x00    /**< CRC preset value is 0x0000 (only used for CRC command) */
#define MFRC522_REG_MODE_CRC_PRESET_6363    0x01    /**< CRC preset value is 0x6363 (only used for CRC command) */
#define MFRC522_REG_MODE_CRC_PRESET_A671    0x02    /**< CRC preset value is 0x6363 (only used for CRC command) */
#define MFRC522_REG_MODE_CRC_PRESET_FFFF    0x02    /**< CRC preset value is 0x6363 (only used for CRC command) */
/** @} */

/**
 * @name    Bits in the @ref MFRC522_REG_ERROR_MASK register
 * @{
 */
#define MFRC522_REG_ERROR_UNEXPECTED_WRITE  BIT7    /**< Unexpected write to FIFO */
#define MFRC522_REG_ERROR_OVERHEATED        BIT6    /**< Thermal shutdown of antenna drivers active */
#define MFRC522_REG_ERROR_OVERFLOW          BIT4    /**< FIFO has overflown */
#define MFRC522_REG_ERROR_COLLISION         BIT3    /**< Bit-Collision during anticollision procedure detected */
#define MFRC522_REG_ERROR_CRC               BIT2    /**< Checksum Mismatch */
#define MFRC522_REG_ERROR_PARITY            BIT1    /**< Parity Check Failed */
#define MFRC522_REG_ERROR_PROTOCOL          BIT0    /**< Protocol error during MFAuthent command */
#define MFRC522_REG_ERROR_ANY               0xdf    /**< Mask matching any error */
/** @} */

#define MFRC522_POLLING_TIMEOUT_MS          100U    /**< Time between two REQA commands during polling */
#define MFRC522_FIFO_SIZE                   64U     /**< Size of the TX/RX FIFO in bytes */

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_CONSTANTS_H */
/** @} */
