/*
 * Copyright (C) 2016 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>

#include "fmt.h"
#include "net/nanocoap.h"
#include "dfplayer.h"
#include "mpu9x50.h"
#include "ws281x.h"
#include "demo.h"

#define STRLEN(x) (sizeof(x) - 1)

static ws281x_t *ws281x = &ws281x_dev;
static mpu9x50_t *mpu = &mpu9x50_devs[0];
static const uint8_t *io_error = (uint8_t *)"I/O error";
static const size_t io_error_len = STRLEN("I/O error");
static const uint8_t *no_state_error = (uint8_t *)"Unknown state";
static const size_t no_state_error_len = STRLEN("Unknown state");
static const uint8_t *invalid_error = (uint8_t *)"invalid";
static const size_t invalid_error_len = STRLEN("invalid");
static const uint8_t *server_error = (uint8_t *)"nanocoap";
static const size_t server_error_len = STRLEN("nanocoap");
static const uint8_t *range_error = (uint8_t *)"range";
static const size_t range_error_len = STRLEN("range");
static dfplayer_t *dfp = &dfplayer_devs[0];


static ssize_t cont_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                             void *context)
{
    (void)context;
    unsigned code = COAP_CODE_CONTENT;
    bool cont = atomic_load(&dfp_mode) == DFP_CONTINUOUS;
    uint8_t reply[1];

    switch (coap_method2flag(coap_get_code_detail(pkt))) {
        case COAP_PUT:
            if ((pkt->payload_len < 1) ||
                ((*pkt->payload != '0') && (*pkt->payload != '1')))
            {
                return coap_reply_simple(pkt, COAP_CODE_BAD_REQUEST, buf, len,
                                         COAP_FORMAT_TEXT, invalid_error,
                                         invalid_error_len);
            }

            cont = (*pkt->payload == '1');
            if (cont) {
                atomic_store(&dfp_mode, DFP_CONTINUOUS);
            }
            code = COAP_CODE_CHANGED;
        default:
            break;
    }
    reply[0] = cont ? '1' : '0';

    return coap_reply_simple(pkt, code, buf, len, COAP_FORMAT_TEXT, reply, 1);
}

static ssize_t repeat_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                              void *context)
{
    (void)context;
    unsigned code = COAP_CODE_CONTENT;
    bool rpt = atomic_load(&dfp_mode) == DFP_REPEAT;
    uint8_t reply[1];

    switch (coap_method2flag(coap_get_code_detail(pkt))) {
        case COAP_PUT:
            if ((pkt->payload_len < 1) ||
                ((*pkt->payload != '0') && (*pkt->payload != '1')))
            {
                return coap_reply_simple(pkt, COAP_CODE_BAD_REQUEST, buf, len,
                                         COAP_FORMAT_TEXT, invalid_error,
                                         invalid_error_len);
            }

            rpt = (*pkt->payload == '1');
            if (rpt) {
                atomic_store(&dfp_mode, DFP_REPEAT);
            }
            code = COAP_CODE_CHANGED;
        default:
            break;
    }
    reply[0] = rpt ? '1' : '0';

    return coap_reply_simple(pkt, code, buf, len, COAP_FORMAT_TEXT, reply, 1);
}

static ssize_t state_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                             void *context)
{
    (void)context;
    unsigned code = COAP_CODE_CONTENT;
    int error = 0;
    switch (coap_method2flag(coap_get_code_detail(pkt))) {
        case COAP_PUT:
            if ((pkt->payload_len == STRLEN("play")) &&
                !memcpy(pkt->payload, "play", STRLEN("play")))
            {
                error = dfplayer_play(dfp);
            }
            else if ((pkt->payload_len == STRLEN("pause")) &&
                     !memcpy(pkt->payload, "pause", STRLEN("pause")))
            {
                error = dfplayer_pause(dfp);
            }
            else {
                return coap_reply_simple(pkt, COAP_CODE_BAD_REQUEST, buf, len,
                                         COAP_FORMAT_TEXT, no_state_error,
                                         no_state_error_len);
            }
            code = COAP_CODE_CHANGED;
            break;
        default:
            break;
    }

    if (error) {
        return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len,
                                 COAP_FORMAT_TEXT,
                                 io_error, io_error_len);
    }

    const char *str_state = "error";
    dfplayer_state_t state = DFPLAYER_STATE_NUMOF;
    dfplayer_get_state(dfp, &state);
    switch (state) {
        case DFPLAYER_STATE_PLAYING:
            str_state = "play";
            break;
        case DFPLAYER_STATE_PAUSED:
            str_state = "pause";
            break;
        default:
        case DFPLAYER_STATE_STOPPED:
            str_state = "stop";
            break;
    }

    return coap_reply_simple(pkt, code, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t *)str_state,
                             strlen(str_state));
}

static ssize_t track_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                             void *context)
{
    (void)context;
    char reply[32];
    unsigned code = COAP_CODE_CONTENT;
    int error = 0;
    switch (coap_method2flag(coap_get_code_detail(pkt))) {
        case COAP_PUT:
            if (pkt->payload_len > STRLEN("99/255")) {
                return coap_reply_simple(pkt, COAP_CODE_BAD_REQUEST, buf, len,
                                         COAP_FORMAT_TEXT, invalid_error,
                                         invalid_error_len);
            }
            else {
                char *end;
                memcpy(reply, pkt->payload, pkt->payload_len);
                reply[pkt->payload_len] = '\0';
                int folder = strtol(reply, &end, 10);
                if (end == reply + pkt->payload_len) {
                    error = dfplayer_play_from_mp3(dfp, (uint16_t)folder);
                }
                else {
                    int file = strtol(end + 1, NULL, 10);
                    error = dfplayer_play_file(dfp, (uint8_t)folder,
                                               (uint8_t)file);
                }
            }
            code = COAP_CODE_CHANGED;
            if (error) {
                return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len,
                                         COAP_FORMAT_TEXT,
                                         io_error, io_error_len);
            }
            break;
        default:
            break;
    }

    dfplayer_track_t track = dfplayer_get_track(dfp);

    char *pos = reply;
    if (track.scheme == DFPLAYER_SCHEME_FOLDER_FILE) {
        pos += fmt_u16_dec(pos, track.folder);
        *pos++ = '/';
        pos += fmt_u16_dec(pos, track.file);
    }
    else {
        pos += fmt_u16_dec(pos, track.number);
    }

    size_t reply_len = pos - reply;

    return coap_reply_simple(pkt, code, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t *)reply, reply_len);
}

static ssize_t volume_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                             void *context)
{
    (void)context;
    char reply[32];
    unsigned code = COAP_CODE_CONTENT;
    int error = 0;
    uint8_t volume = 0;
    switch (coap_method2flag(coap_get_code_detail(pkt))) {
        case COAP_PUT:
            if (pkt->payload_len > 3) {
                return coap_reply_simple(pkt, COAP_CODE_BAD_REQUEST, buf, len,
                                         COAP_FORMAT_TEXT, invalid_error,
                                         invalid_error_len);
            }
            memcpy(reply, pkt->payload, pkt->payload_len);
            reply[pkt->payload_len] = '\0';
            volume = atoi(reply);
            error = dfplayer_set_volume(dfp, volume);
            code = COAP_CODE_CHANGED;
            break;
        default:
            error = dfplayer_get_volume(dfp, &volume);
            break;
    }

    if (error) {
        return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len,
                                 COAP_FORMAT_TEXT,
                                 io_error, io_error_len);
    }


    char *pos = reply;
    pos += fmt_u16_dec(pos, volume);

    size_t reply_len = pos - reply;

    return coap_reply_simple(pkt, code, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t *)reply, reply_len);
}

static ssize_t accel_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                             void *context)
{
    (void)context;
    char reply[32];
    mpu9x50_results_t res;
    if (mpu9x50_read_accel(mpu, &res)) {
        return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len,
                                 COAP_FORMAT_TEXT, io_error, io_error_len);
    }

    char *pos = reply;
    *pos++ = '[';
    pos += fmt_s16_dec(pos, res.x_axis);
    *pos++ = ',';
    *pos++ = ' ';
    pos += fmt_s16_dec(pos, res.y_axis);
    *pos++ = ',';
    *pos++ = ' ';
    pos += fmt_s16_dec(pos, res.z_axis);
    *pos++ = ']';
    *pos++ = ' ';
    *pos++ = 'm';
    *pos++ = 'G';
    size_t reply_len = pos - reply;
    return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t *)reply, reply_len);
}

static ssize_t compass_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                               void *context)
{
    (void)context;
    char reply[32];
    mpu9x50_results_t res;
    if (mpu9x50_read_compass(mpu, &res)) {
        return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len,
                                 COAP_FORMAT_TEXT, io_error, io_error_len);
    }

    char *pos = reply;
    *pos++ = '[';
    pos += fmt_s16_dec(pos, res.x_axis);
    *pos++ = ',';
    *pos++ = ' ';
    pos += fmt_s16_dec(pos, res.y_axis);
    *pos++ = ',';
    *pos++ = ' ';
    pos += fmt_s16_dec(pos, res.z_axis);
    *pos++ = ']';
    *pos++ = ' ';
    *pos++ = 0xc2; /*<- UTF-8 for micro */
    *pos++ = 0xb5;
    *pos++ = 'T';
    size_t reply_len = pos - reply;
    return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t *)reply, reply_len);
}

static ssize_t gyro_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                            void *context)
{
    (void)context;
    char reply[32];
    mpu9x50_results_t res;
    if (mpu9x50_read_gyro(mpu, &res)) {
        return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len,
                                 COAP_FORMAT_TEXT, io_error, io_error_len);
    }

    char *pos = reply;
    *pos++ = '[';
    pos += fmt_s16_dec(pos, res.x_axis);
    *pos++ = ',';
    *pos++ = ' ';
    pos += fmt_s16_dec(pos, res.y_axis);
    *pos++ = ',';
    *pos++ = ' ';
    pos += fmt_s16_dec(pos, res.z_axis);
    *pos++ = ']';
    *pos++ = ' ';
    *pos++ = 'd';
    *pos++ = 'p';
    *pos++ = 's';
    size_t reply_len = pos - reply;
    return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t *)reply, reply_len);
}

static ssize_t temp_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                            void *context)
{
    (void)context;
    char reply[32];
    int32_t res;
    if (mpu9x50_read_temperature(mpu, &res)) {
        return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len,
                                 COAP_FORMAT_TEXT, io_error, io_error_len);
    }

    char *pos = reply;
    pos += fmt_s32_dec(pos, res);
    *pos++ = ' ';
    *pos++ = 'm';
    *pos++ = 0xc2; /*<- UTF-8 for ° */
    *pos++ = 0xb0;
    *pos++ = 'C';
    size_t reply_len = pos - reply;
    return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t *)reply, reply_len);
}

static ssize_t riot_board_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                                  void *context)
{
    (void)context;
    return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t*)RIOT_BOARD,
                             strlen(RIOT_BOARD));
}

static ssize_t ws281x_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                              void *context)
{
    (void)context;
    char uri[NANOCOAP_URI_MAX];
    ssize_t uri_len = coap_get_uri_path(pkt, (uint8_t *)uri);
    if (uri_len <= 0) {
        return coap_reply_simple(pkt, COAP_CODE_INTERNAL_SERVER_ERROR, buf,
                                 len, COAP_FORMAT_TEXT, server_error,
                                 server_error_len);
    }

    char *sub_uri = NULL;
    if (uri[STRLEN("/ws281x")] == '/') {
        sub_uri = uri + STRLEN("/ws281x/");
    }

    if (pkt->payload_len != STRLEN("#ffffff") ||
        pkt->payload[0] != (uint8_t)'#')
    {
        return coap_reply_simple(pkt, COAP_CODE_BAD_REQUEST, buf, len,
                                 COAP_FORMAT_TEXT, no_state_error,
                                 no_state_error_len);
    }

    uint8_t col[3];
    for (unsigned i = 0; i < 3; i++) {
        unsigned idx = 2 * i + 1;
        if (pkt->payload[idx] <= '9') {
            col[i] = pkt->payload[idx] - '0';
        }
        else if (pkt->payload[idx] <= 'F') {
            col[i] = pkt->payload[idx] - 'A' + 10;
        }
        else {
            col[i] = pkt->payload[idx] - 'a' + 10;
        }
        col[i] <<= 4;
        idx++;
        if (pkt->payload[idx] <= '9') {
            col[i] |= pkt->payload[idx] - '0';
        }
        else if (pkt->payload[idx] <= 'F') {
            col[i] |= pkt->payload[idx] - 'A' + 10;
        }
        else {
            col[i] |= pkt->payload[idx] - 'a' + 10;
        }
    }

    color_rgb_t col_rgb = { .r = col[0], .g = col[1], .b = col[2] };

    if (sub_uri) {
        uint16_t idx = atoi((char *)sub_uri);
        if (idx >= ws281x->params.numof) {
            return coap_reply_simple(pkt, COAP_CODE_PATH_NOT_FOUND, buf, len,
                                     COAP_FORMAT_TEXT, range_error,
                                     range_error_len);
        }
        ws281x_set(ws281x, idx, col_rgb);
    }
    else {
        for (uint16_t i = 0; i < ws281x->params.numof; i++) {
            ws281x_set(ws281x, i, col_rgb);
        }
    }

    ws281x_write(ws281x);
    return coap_build_reply(pkt, COAP_CODE_CHANGED, buf, len, 0);
}

/* must be sorted by path (ASCII order) */
const coap_resource_t coap_resources[] = {
    COAP_WELL_KNOWN_CORE_DEFAULT_HANDLER,
    { "/dfplayer/cont", COAP_GET | COAP_PUT, cont_handler, NULL },
    { "/dfplayer/repeat", COAP_GET | COAP_PUT, repeat_handler, NULL },
    { "/dfplayer/state", COAP_GET | COAP_PUT, state_handler, NULL },
    { "/dfplayer/track", COAP_GET | COAP_PUT, track_handler, NULL },
    { "/dfplayer/volume", COAP_GET | COAP_PUT, volume_handler, NULL },
    { "/mpu9250/accel", COAP_GET, accel_handler, NULL },
    { "/mpu9250/compass", COAP_GET, compass_handler, NULL },
    { "/mpu9250/gyro", COAP_GET, gyro_handler, NULL },
    { "/mpu9250/temp", COAP_GET, temp_handler, NULL },
    { "/riot/board", COAP_GET, riot_board_handler, NULL },
    { "/ws281x", COAP_PUT | COAP_MATCH_SUBTREE, ws281x_handler, NULL },
};

const unsigned coap_resources_numof = ARRAY_SIZE(coap_resources);
