#include <stdio.h>

#include "atomic_utils.h"
#include "byteorder.h"
#include "event.h"
#include "event/thread.h"
#include "log.h"
#include "luid.h"
#include "net/ptp.h"
#include "net/sock/async/event.h"
#include "net/sock/udp.h"
#include "periph/ptp.h"
#include "random.h"
#include "timex.h"

#define ENABLE_DEBUG        0
#include "debug.h"

/*
 * IMPLEMENTATION NOTE: This code is not thread safe. It relies instead on the
 * fact that a single event queue (and thus, a single thread) is used throughout
 * the implementation. Changing this architecture should be done with care.
 * (Or better: Not all all ;-))
 */

/* Event queue to use */
#define PTP_EVENT_QUEUE         EVENT_PRIO_MEDIUM

enum ptp_state {
    STATE_IDLE,                     /**< Not currently synchronizing */
    STATE_WAIT_FOR_FOLLOW_UP,       /**< Got two-step sync message, waiting for follow up */
    STATE_WAIT_FOR_DELAY_RESP,      /**< Sent delay request, waiting for delay resp */
};

/**
 * @brief   Check if the given PTP server matches the one selected as
 *          source
 *
 * @retval  1       Given server is the selected one
 * @retval  0       Given server is different to selected one
 */
static int is_selected_ptp_server(const ptp_hdr_t *hdr);

/**
 * @brief   Timer callback used to post the timer event
 */
static void timer_callback(void *arg);

/**
 * @brief   Extract the timestamp for a header and return it as nanoseconds
 *          since epoch
 */
static uint64_t parse_timestamp(const ptp_hdr_t *hdr);

/**
 * @brief   Timer event handler used for house keeping
 *
 * This mainly used to update the estimation of the network delay
 */
static void timer_event_handler(event_t *ev);

/**
 * @brief   Send a delay request
 *
 * This sends a delay requests, updates the state, and stores the TX timestamp
 * in `time_last`
 */
static void send_delay_req(void);

/* Sockets to listen for PTP events, and PTP general messages (distinct UDP ports ) */
static sock_udp_t sock_event, sock_general;
static enum ptp_state state = STATE_IDLE;
static uint16_t sequence_id = 0;
static uint16_t delay_req_sequence_id = 0;
static uint64_t time_last = 0;
static uint64_t last_sync = 0;
static uint8_t server_prio = UINT8_MAX;
static xtimer_t timer = { .callback = timer_callback };
static event_t timer_event = { .handler = timer_event_handler };
/* Time between two delay requests in µs (a pseudorandom offset will be added on
 * top to distribute the delay requests */
static const uint32_t delay_req_interval = 10 * US_PER_SEC;
/* Timeout for a delay request */
static const uint32_t delay_req_timeout = US_PER_SEC / 2;
static const sock_udp_ep_t ep_ptp_primary_event = {
    .family = AF_INET6,
    .addr.ipv6 = {
        0xff, 0x0e, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x01, 0x81,
    },
    .netif = SOCK_ADDR_ANY_NETIF,
    .port = PTP_PORT_EVENT,
};

/* Provide symbols for these, so that the shell application can access them */
uint32_t ptp_rtt = 0;
uint16_t ptp_utc_offset = 0;
int32_t ptp_clock_drift = 0;
ptp_clock_id_t ptp_local_clock_id;
ptp_clock_id_t ptp_server_clock_id;

static void adjust_time(uint64_t server_time, uint64_t local_time)
{
    int64_t offset_ns = (int64_t)server_time - (int64_t)local_time;
    offset_ns += ptp_rtt / 2;
    ptp_clock_adjust(offset_ns);
    DEBUG("[ptp_client] Adjusted time by %ld ns\n", (long)offset_ns);

    if (last_sync) {
        DEBUG("[ptp_client] clock drifted by %ld ns during %lu ns\n",
              (long)offset_ns, (unsigned long)(server_time - last_sync));
        /* rely on compiler to use arithmetic shifts when possible */
        offset_ns *= (1LL << 32);
        int32_t tmp = offset_ns / (int64_t)(server_time - last_sync);
        /* Smooth out jumps in clock drift compensation to avoid overshooting by reducing steps.
         * But do the big jump right away on boot, to reduce settling time */
        if (ptp_clock_drift) {
            tmp = (tmp / 8) + ptp_clock_drift;
        }
        if ((tmp < -42949673) || (tmp > 42949673)) {
            DEBUG("[ptp_client] Estimated clock drift of %ld not plausible, resetting it.\n",
                  (long)tmp);
            tmp = 0;
        }
        atomic_store_u32((uint32_t *)&ptp_clock_drift, (uint32_t)(int32_t)tmp);
        if (IS_USED(MODULE_PERIPH_PTP_SPEED_ADJUSTMENT)) {
            ptp_clock_adjust_speed(tmp);
        }
    }
    last_sync = server_time;
}

static void adjust_rtt(uint64_t sent, uint64_t received)
{
    /* undo RTT compensation for sent timestamp */
    sent -= ptp_rtt / 2;
    uint32_t tmp = received - sent;
    if (tmp > 200000) {
        DEBUG("[ptp_client] RTT estimation of %lu not plausible, resetting it.\n",
              (unsigned long)tmp);
        tmp = 0;
    }
    else {
        /* Reduce jumps in RTT estimation by averaging in old RTT estimation, if any */
        if (ptp_rtt) {
            tmp = (3 * ptp_rtt + tmp) >> 2;
        }
    }
    /* Note: we can safely read without atomic access form ptp_rtt here, as this thread is the
     * only one allowed to write to it. But we need to atomically write to it, so that atomic
     * reads from it by external threads (such as the ptp shell command) never read corrupted
     * data */
    atomic_store_u32(&ptp_rtt, tmp);

    /* Do not estimate clock drift right after RTT has changed */
    last_sync = 0;
}

static void set_timer(uint32_t interval)
{
    xtimer_remove(&timer);
    /* Add random offset between 0 s and 1.048575 s, so that delay requests
     * of many clients don't synchronize and overload the server */
    interval += random_uint32() & 0xfffff;
    DEBUG("[ptp_client] Next timeout in %u.%06us\n", (unsigned)(interval / US_PER_SEC),
          (unsigned)(interval % US_PER_SEC));
    xtimer_set(&timer, interval);
}

static void timer_callback(void *arg)
{
    (void)arg;
    event_post(PTP_EVENT_QUEUE, &timer_event);
}

static void timer_event_handler(event_t *ev)
{
    (void)ev;
    if (state == STATE_WAIT_FOR_DELAY_RESP) {
        DEBUG("[ptp_client] Delay response timed out, sending new request\n");
    }

    if (state == STATE_WAIT_FOR_FOLLOW_UP) {
        DEBUG("[ptp_client] Wait for follow up prior to sending delay request\n");
        /* We can just reuse the delay request timeout here */
        set_timer(delay_req_timeout);
    }

    send_delay_req();

    /* Increment server prio occasionally. It will be reset when a new announce
     * message is received. This way, the PTP client will eventually switch
     * form a high prio server to a low prio back up server, in case the
     * high prio server stops announcing. This algorithm is much simpler than
     * the correct "best master clock" algorithm, but should still work well
     * for the vast majority of use cases.
     */
    server_prio++;
}

static uint64_t parse_timestamp(const ptp_hdr_t *hdr)
{
    const uint8_t *s = hdr->time_seconds;
    /* No ntoh<FOO> for 48 bit integers, thus doing conversion by hand */
    uint64_t secs = (uint64_t)s[0] << 40 | (uint64_t)s[1] << 32 |
                    (uint64_t)s[2] << 24 | (uint64_t)s[3] << 16 |
                    (uint64_t)s[4] << 8  | (uint64_t)s[5];
    uint32_t ns = ntohl(hdr->time_nanoseconds);
    return secs * NS_PER_SEC + ns;
}

static int is_selected_ptp_server(const ptp_hdr_t *hdr)
{
    return !(memcmp(ptp_server_clock_id.bytes, hdr->clock_identity,
                    sizeof(ptp_server_clock_id)));
}

static void send_delay_req(void)
{
    ptp_hdr_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.message_type = PTP_MSG_TYPE_DELAY_REQ;
    msg.ptp_version_major = 2;
    msg.ptp_version_minor = 0;
    msg._obsolete_control = PTP_DELAY_REQ_CONTROL;
    msg.log_msg_interval = PTP_DELAY_REQ_LOG_INTERVAL;
    msg.length = htons(sizeof(msg));
    msg.sequence_id = htons(++delay_req_sequence_id);
    memcpy(msg.clock_identity, ptp_local_clock_id.bytes, sizeof(ptp_local_clock_id.bytes));
    msg.source_port_id = htons(1);

    sock_udp_aux_tx_t aux = { .flags = SOCK_AUX_GET_TIMESTAMP };
    ssize_t retval = sock_udp_send_aux(&sock_general, &msg, sizeof(msg),
                                       &ep_ptp_primary_event, &aux);
    if (retval < 0) {
        LOG_ERROR("[ptp_client] send failed\n");
        set_timer(delay_req_interval);
    }

    if (aux.flags & SOCK_AUX_GET_TIMESTAMP) {
        LOG_WARNING("[ptp_client] no TX timestamp, cannot determine network delay\n");
        state = STATE_IDLE;
        set_timer(delay_req_interval);
    }
    else {
        time_last = aux.timestamp;
        state = STATE_WAIT_FOR_DELAY_RESP;
        DEBUG("[ptp_client] Waiting for delay response\n");
        set_timer(delay_req_timeout);
    }
}

static int handle_announce(const ptp_msg_announce_t *msg)
{
    if (is_selected_ptp_server(&msg->hdr)) {
        /* Update server priority, as
         * a) It might have been changed by the admin
         * b) We increment (decrease) the priority periodically, so that
         *    the PTP client eventually switches to a new PTP server if the
         *    old becomes unresponsive. Thus, we need to restore the priority
         *    while the server is still responsive. The announce messages are
         *    used to track "aliveness" of the PTP server.
         */
        server_prio = msg->priority1;
    }
    else {
        DEBUG("[ptp_client] Got announce from new server\n");
        if (msg->priority1 < server_prio) {
            DEBUG("[ptp_client] Switching to new PTP server\n");
            state = STATE_IDLE;
            memcpy(&ptp_server_clock_id, msg->hdr.clock_identity,
                   sizeof(ptp_server_clock_id));
            server_prio = msg->priority1;
            /* reset network delay; it is likely different from the value of
             * the old server */
            atomic_store_u32(&ptp_rtt, 0);
            atomic_store_u16(&ptp_utc_offset, ntohs(msg->utc_offset));
            /* trigger a network delay measurement */
            set_timer(delay_req_interval);
        }
    }
    return 0;
}

static int handle_msg(const ptp_hdr_t *hdr, size_t length, uint64_t timestamp)
{

    if ((hdr->ptp_version_major != 2) || (hdr->ptp_version_minor > 1)) {
        DEBUG("[ptp_client] Not PTP version 2.0 / 2.1\n");
        return -ENOTSUP;
    }

    if (length < sizeof(ptp_hdr_t)) {
        DEBUG("[ptp_client] Message invalid, too small\n");
        return -EBADMSG;
    }

    switch (hdr->message_type) {
    case PTP_MSG_TYPE_SYNC:
        if (is_selected_ptp_server(hdr)) {
            sequence_id = ntohs(hdr->sequence_id);
            uint16_t flags = ntohs(hdr->flags);
            DEBUG("[ptp_client] Got sync with ID %u and flags 0x%x\n",
                  (unsigned)sequence_id, (unsigned)flags);
            if (!(flags & PTP_FLAG_TWO_STEP)) {
                /* Without two step flag, the sync already contains a precise
                 * hardware supplied timestamp and no follow up is send by the
                 * server */
                adjust_time(parse_timestamp(hdr), timestamp);
                state = STATE_IDLE;
                return 0;
            }
            /* Two step sync: A follow up message will contain the precise
             * timestamp of when this sync was send */
            time_last = timestamp;
            state = STATE_WAIT_FOR_FOLLOW_UP;
        }
        break;
    case PTP_MSG_TYPE_FOLLOW_UP:
        if (is_selected_ptp_server(hdr) &&
            (state == STATE_WAIT_FOR_FOLLOW_UP)) {
            uint16_t _sequence_id = ntohs(hdr->sequence_id);
            if (_sequence_id != sequence_id) {
                DEBUG("[ptp_client] Ignoring follow up message with unexpected "
                      "sequence id\n");
                return 0;
            }
            DEBUG("[ptp_client] Got follow up for ID %u\n", (unsigned)_sequence_id);
            adjust_time(parse_timestamp(hdr), time_last);
            state = STATE_IDLE;
            return 0;
        }
        DEBUG("[ptp_client] Ignoring unexpected follow up\n");
        break;
    case PTP_MSG_TYPE_DELAY_RESP:
        if (is_selected_ptp_server(hdr) &&
            (state == STATE_WAIT_FOR_DELAY_RESP)) {
            if (length < sizeof(ptp_msg_delay_resp_t)) {
                DEBUG("[ptp_client] Delay response message invalid, too small\n");
                return -EBADMSG;
            }
            ptp_msg_delay_resp_t *resp = (ptp_msg_delay_resp_t *)hdr;
            if (memcmp(resp->client_clock_identity, ptp_local_clock_id.bytes,
                       sizeof(ptp_local_clock_id.bytes))) {
                DEBUG("[ptp_client] Ignoring delay response intended for other "
                      "client\n");
                return 0;
            }
            uint16_t _sequence_id = ntohs(hdr->sequence_id);
            if (_sequence_id != delay_req_sequence_id) {
                DEBUG("[ptp_client] Ignoring follow up message with unexpected "
                      "sequence id\n");
                return 0;
            }
            adjust_rtt(time_last, parse_timestamp(hdr));
            state = STATE_IDLE;
            set_timer(delay_req_interval);
            return 0;
        }
        DEBUG("[ptp_client] Ignoring unexpected delay response\n");
        break;
    case PTP_MSG_TYPE_ANNOUNCE:
        /* PTP server selection ("best master clock algorithm") not implemented,
         * so we just ignore announce messages */
        if (length < sizeof(ptp_msg_announce_t)) {
            DEBUG("[ptp_client] Announce message invalid, too small\n");
            return -EBADMSG;
        }
        return handle_announce((ptp_msg_announce_t *)hdr);
    default:
        DEBUG("[ptp_client] Ignoring unhandled message PTP message type\n");
        break;
    }

    return 0;
}

static void ptp_handler(sock_udp_t *sock, sock_async_flags_t type, void *arg)
{
    (void)arg;
    static uint8_t buf[128];
    if (type & SOCK_ASYNC_MSG_RECV) {
        sock_udp_aux_rx_t aux = { .flags = SOCK_AUX_GET_TIMESTAMP };
        ssize_t res;
        if (0 <= (res = sock_udp_recv_aux(sock, buf, sizeof(buf), 0,
                                          NULL, &aux))) {
            ptp_hdr_t *hdr = (ptp_hdr_t *)buf;
            if (aux.flags & SOCK_AUX_GET_TIMESTAMP) {
                /* Keeping log message short, as even on machines with little
                 * flash, error messages are usually compiled in. Without
                 * RX timestamp, no PTP synchronization is possible */
                LOG_ERROR("[ptp_client] No RX timestamp\n");
                return;
            }
            if (ntohs(hdr->length) > res) {
                DEBUG("[ptp_client] Length of PTP header is %u but UDP payload is only  %u\n",
                      (unsigned)ntohs(hdr->length), (unsigned)res);
                return;
            }
            handle_msg(hdr, (size_t)res, aux.timestamp);
        }
    }
}

int search_and_prepare_netif(sock_udp_ep_t *local_event,
                             sock_udp_ep_t *local_general)
{
    gnrc_netif_t *netif = NULL;

    while (1) {
        ipv6_addr_t ipv6_addrs[CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF];
        netif = gnrc_netif_iter(netif);
        if (!netif) {
            return -ENODEV;
        }
        int res = gnrc_netapi_get(netif->pid, NETOPT_IPV6_ADDR, 0, ipv6_addrs,
                                  sizeof(ipv6_addrs));
        if (res < 1) {
            continue;
        }

        memcpy(local_event->addr.ipv6, ipv6_addrs[0].u8,
               sizeof(local_event->addr.ipv6));
        memcpy(local_general->addr.ipv6, ipv6_addrs[0].u8,
               sizeof(local_general->addr.ipv6));
        if (0 > netif_set_opt(&netif->netif, NETOPT_IPV6_GROUP, 0,
                              (void *)ep_ptp_primary_event.addr.ipv6,
                              sizeof(ep_ptp_primary_event.addr.ipv6))) {
            return -EADDRNOTAVAIL;
        }

        return 0;
    }
}

int ptp_start_client(void)
{
    sock_udp_ep_t local_event = {
        .family = AF_INET6, .port = PTP_PORT_EVENT
    };
    sock_udp_ep_t local_general = {
        .family = AF_INET6, .port = PTP_PORT_GENERAL
    };

    int retval = search_and_prepare_netif(&local_event, &local_general);
    if (retval) {
        return retval;
    }

    if (sock_udp_create(&sock_event, &local_event, NULL, 0)) {
        return -ENOTCONN;
    }

    if (sock_udp_create(&sock_general, &local_general, NULL, 0)) {
        sock_udp_close(&sock_event);
        return -ENOTCONN;
    }

    luid_base(ptp_local_clock_id.bytes, sizeof(ptp_local_clock_id.bytes));
    sock_udp_event_init(&sock_event, PTP_EVENT_QUEUE, ptp_handler, NULL);
    sock_udp_event_init(&sock_general, PTP_EVENT_QUEUE, ptp_handler, NULL);

    return 0;
}
