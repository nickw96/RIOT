/*
 * Copyright (C) 2020 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for
 * more details.
 */

/**
 * @defgroup    net_ptp     Precision Time Protocol (PTP)
 * @ingroup     net
 * @brief       Implementation of the Precision Time Protocol (PTP)
 *
 * Usage PTP Client
 * ================
 *
 * The simplest way to use the PTP client is by adding
 * `USEMODULE += auto_init_ptp_client` to the applications Makefile. This
 * will launch the PTP client upon boot and listen on IPv6 multicast group
 * `ff0e::181`. Note that the client does not implement the "best master clock"
 * algorithm for the selection of the PTP server. Instead, the client will
 * simply use the PTP server with the highest priority 1 (lowest numeric value).
 * The client will internally count the priority of the currently selected
 * server up (lower its priority) using a timer, but restore the priority
 * whenever a new announce message is received. This way, an unresponsive
 * high priority PTP server will eventually be replaced by a lower priority
 * back up server. However, in order to avoid the client to constantly jump
 * between servers, the servers should send announce messages at least every
 * ten seconds. (Alternatively, adding some "guard space" between the used
 * priority values would also work.)
 *
 * @{
 *
 * @file
 * @brief       PTP interface and type definitions
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */


#ifndef NET_PTP_H
#define NET_PTP_H

#include <stdint.h>
#include <string.h>

#include "atomic_utils.h"
#include "irq.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Common PTP header used by all message types
 *
 * @note    All numbers are in network byte order
 */
typedef struct __attribute__((packed)) {
    uint8_t message_type        : 4;    /**< Type of the PTP message */
    uint8_t major_sdo_id        : 4;    /**< Subdomain number, major part */
    uint8_t ptp_version_major   : 4;    /**< PTP version (major part) */
    uint8_t ptp_version_minor   : 4;    /**< PTP version (minor part) */
    /**
     * @brief  Length of the PTP message in bytes (network byte order)
     *
     * The length includes this header.
     */
    uint16_t length;
    uint8_t domain_number;              /**< domain number of the originating clock */
    uint8_t minor_sdo_id;               /**< See ptp_hdr_t::major_sdo_id */
    uint16_t flags;                     /**< Flags */
    uint8_t correction[8];              /**< Used to convert PTP time to residence time */
    uint8_t type_specific[4];           /**< Contents depend on message type field */
    uint8_t clock_identity[8];          /**< Identifier of the PTP clock */
    uint16_t source_port_id;            /**< Id of the port */
    uint16_t sequence_id;               /**< Useful to match follow up and sync message */
    uint8_t _obsolete_control;          /**< The control field is obsolete */
    /**
     * @brief   When carried in a sync message: The interval the server sends sync message
     *
     * @warning This field has different meaning depending on message type
     */
    uint8_t log_msg_interval;
    uint8_t time_seconds[6];            /**< Timestamp in seconds (48 bit, network byte order) */
    uint32_t time_nanoseconds;          /**< Nanoseconds part of the timestamp (network byte order)*/
} ptp_hdr_t;

/**
 * @brief   Wire format of an PTP announce message
 *
 * @note    All numbers are in network byte order
 */
typedef struct __attribute__((packed)) {
    ptp_hdr_t hdr;                      /**< PTP common header */
    uint16_t utc_offset;                /**< Offset between UTC and TAI (due to leap seconds) in seconds */
    uint8_t _reserved;                  /**< Reserved for future use */
    uint8_t priority1;                  /**< Administrator assigned priority of the clock (lower number = higher prio */
    uint8_t clock_quality[4];           /**< Quality of the clock */
    uint8_t priority2;                  /**< Administrator assigned priority, see also ptp_msg_announce_t::priority1 */
    uint8_t identity[8];                /**< Identity of the grandmaster clock */
    uint16_t steps_removed;             /**< Distance to the grandmaster clock in number of communication paths */
    uint8_t time_source;                /**< Time source used, @see ptp_time_source_t */
} ptp_msg_announce_t;

/**
 * @brief   Wire format of an PTP delay response message
 */
typedef struct __attribute__((packed)) {
    ptp_hdr_t hdr;                      /**< PTP common header */
    uint8_t client_clock_identity[8];   /**< Identifier of the PTP clock of the requesting client */
    uint16_t client_source_port_id;     /**< Id of the port of the requesting client */
} ptp_msg_delay_resp_t;

/**
 * @brief   Type of PTP clock ID
 */
typedef struct {
    uint8_t bytes[8];                   /**< The opaque byte array identifying a clock */
} ptp_clock_id_t;

/**
* @brief   Enumeration of time sources in the PTP protocol
*/
typedef enum {
    PTP_SOURCE_ATOMIC_CLOCK = 0x10,     /**< Atomic clock is time source */
    PTP_SOURCE_GNSS         = 0x20,     /**< From satellite, e.g. GPS time */
    PTP_SOURCE_RADIO        = 0x30,     /**< From radio signal, e.g. DCF77 */
    PTP_SOURCE_SERIAL       = 0x39,     /**< From serial interface, e.g. IRIG interface of atomic clock */
    PTP_SOURCE_PTP          = 0x40,     /**< From other PTP clock */
    PTP_SOURCE_NTP          = 0x50,     /**< From (S)NTP server */
    PTP_SOURCE_HAND_SET     = 0x60,     /**< Manually entered time from biological life form */
    PTP_SOURCE_OTHER        = 0x70,     /**< Other sources */
    PTP_SOURCE_OSCIALLATOR  = 0xA0,     /**< Internal oscillator */
} ptp_time_source_t;

/**
* @brief   PTP Message Types
*
* @see ptp_hdr_t::message_type
*/
typedef enum {
    /**
     * @brief   Sync-Message
     *
     * This message is send from the server (UDP port 319) to the client
     * (UDP Port 319) to start a time synchronization, usually via multicast
     * (IPv6 group `ff0e::181`). When the two step flag is set, it contains a
     * bogus time stamp and the follow up message contains the time stamp the
     * server send the previous sync message. If the flag is not set, the
     * time stamp was generated by the sending Ethernet card on the fly and
     * matches the exact time the start of frame delimiter was send.
     */
    PTP_MSG_TYPE_SYNC       = 0x0,
    /**
     * @biref   Follow Up Message
     *
     * This message is send from the server (UDP port 320) to the client
     * (UDP Port 320) directly after the Sync-Message, if the Sync-Message did
     * not contain a precise time stat. The time stamp it contains refers to
     * the time the start of frame delimiter of the corresponding Sync-Message
     * was send. This message is usually send via multicast
     * (IPv6 group `ff0e::181`).
     */
    PTP_MSG_TYPE_FOLLOW_UP  = 0x8,
    /**
     * @brief   Delay Request Message
     *
     * This message is send from the client (UDP port 319) to the server
     * (UDP Port 319) via unicast and is used to estimate the network delay
     * (assuming it is symmetric).
     */
    PTP_MSG_TYPE_DELAY_REQ  = 0x1,
    /**
     * @brief   Delay Response Mssage
     *
     * This message is send as reply to a delay request message from the
     * server (UDP port 320) to the client (UDP port 320) via unicast. It
     * contains the precise time stamp the matching Delay Request was received
     * (more specifically, the start of frame delimiter).
     */
    PTP_MSG_TYPE_DELAY_RESP = 0x9,
    PTP_MSG_TYPE_ANNOUNCE   = 0xb, /**< PTP server announce message */
} ptp_msg_type_t;

/**
 * @name    Flags used in the @ref ptp_hdr_t::flags
 * @{
 */
#define PTP_FLAG_UNICAST            (0x0400)    /**< PTP Server operates in unicast mode */
#define PTP_FLAG_TWO_STEP           (0x0200)    /**< Expect Follow Up message for Sync message */
#define PTP_FLAG_UTC_OFFSET_VALID   (0x0004)    /**< UTC offset stated in announce message is valid */
/** @} */

/**
 * @name    Magic values to put into a Delay_Req
 * @{
 */
#define PTP_DELAY_REQ_CONTROL       (1)         /**< Value to put into the obsolete control field */
#define PTP_DELAY_REQ_LOG_INTERVAL  (0x7f)      /**< See table 42 in the PTP standard */
/** @} */

/**
 * @name    Port numbers used in PTP
 * @{
 */
#define PTP_PORT_EVENT              (319)       /**< Port for event messages */
#define PTP_PORT_GENERAL            (320)       /**< Port for general messages */
/** @} */

/**
 * @name    Access variables of the current state of the PTP client
 * @{
 */
/**
 * @brief   Get the estimated round trip network delay
 *
 * This is the sum of the time between sending the Ethernet start of frame
 * delimiter and receiving it for both directions
 */
static inline uint32_t ptp_get_rtt(void)
{
    extern uint32_t ptp_rtt;
    return atomic_load_u32(&ptp_rtt);
}
/**
 * @brief   Get the current offset to UTC time in seconds
 *
 * This value is taken from the announce message of the PTP server
 */
static inline uint16_t ptp_get_utc_offset(void)
{
    extern uint16_t ptp_utc_offset;
    return atomic_load_u16(&ptp_utc_offset);
}
/**
 * @brief   The clock ID of the client
 *
 * This value is obtained from luid_base during @ptp_start_client and remains constant.
 */
extern ptp_clock_id_t ptp_local_clock_id;

/**
 * @brief   Get the clock ID of the selected server
 */
static inline void ptp_get_server_clock_id(ptp_clock_id_t *dest)
{
    extern ptp_clock_id_t ptp_server_clock_id;
    unsigned irq_state = irq_disable();
    memcpy(dest, &ptp_server_clock_id, sizeof(*dest));
    irq_restore(irq_state);
}
/**
 * @brief   Get the current clock drift relative to the reference clock of the PTP server
 *
 * This uses the format required by ptp_clock_adjust_speed()
 */
static inline int32_t ptp_get_clock_drift(void)
{
    extern int32_t ptp_clock_drift;
    return (int32_t)atomic_load_u32((uint32_t *)&ptp_clock_drift);
}
/** @} */

/**
* @brief   Launch a rudimentary PTP client on the first netif with an IPv6 addr
*
* This function will take the first network interface with an IPv6 address,
* will join the link local primary PTP IPv6 multicast group, and start an PTP
 * client in the medium priority event handler thread.
 *
 * @warning The client does not implement the "best master clock (BMC)"
 *          algorithm. Instead, only the priority 1 field of the announce
 *          message is evaluated. It is strongly advised to not use this
 *          client when multiple PTP servers have the same priortiy 1 value.
 *
 * @retval  -ENODEV         No network interface with IPv6 address found
 * @retval  -EADDRNOTAVAIL  Failed to join primary PTP IPv6 multicast group
 * @retval  -ENOTCONN       Failed to create UDP socket
 * @retval  0               Success, PTP client listening for Sync messages
 */
int ptp_start_client(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_PTP_H */
/** @} */
