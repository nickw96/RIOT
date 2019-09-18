#include <stdio.h>

#include "dfplayer.h"
#include "dfplayer_constants.h"
#include "mpu9150.h"
#include "mpu9150_params.h"
#include "neopixel.h"
#include "neopixel_params.h"
#include "xtimer.h"

#include "rss.h"

/* Accelerometer threshold for changing the volume */
#define THR_ACC_VOLUME      250
/* Accelerometer threshold selecting a song */
#define THR_ACC_SELECT      250
/* Accelerometer threshold for toggling between play and pause */
#define THR_ACC_PAUSE      -700

/* Time to wait after changing the volume */
#define VOLUME_DELAY_MS     100
/* Time to wait after selecting a song */
#define SELECT_DELAY_MS     500
/* Time to wait before displaying the next animation step */
#define ANIM_DELAY_MS       100

static neopixel_t neo;
static mpu9150_t mpu;

static const color_rgb_t green  = { .r = 0x00, .g = 0xff, .b = 0x00 };
static const color_rgb_t black  = { .r = 0x00, .g = 0x00, .b = 0x00 };
static const color_rgb_t blue   = { .r = 0x00, .g = 0x00, .b = 0xff };
//static const color_rgb_t yellow = { .r = 0xff, .g = 0xff, .b = 0x00 };
static const color_rgb_t red    = { .r = 0xff, .g = 0x00, .b = 0x00 };

static void set_volume(uint8_t volume)
{
    if (dfplayer_set_volume(dfplayer_get(0), volume)) {
        puts("Error: Failed to set volume");
    }

    uint32_t val = volume * neo.params.numof * 256 / 30;
    for (uint16_t i = 0; i < neo.params.numof; i++) {
        color_rgb_t col = { .r = 0, .g = 0, .b = 0 };
        if (val > 255) {
            col.r = 255;
            val -= 256;
        }
        else {
            col.r = (uint8_t)val;
            val = 0;
        }
        neopixel_set(&neo, i, col);
    }

    neopixel_write(&neo);

    xtimer_usleep(VOLUME_DELAY_MS * US_PER_MS);
}

static void step(int step)
{
    color_rgb_t left, right;
    switch (dfplayer_step(dfplayer_get(0), step)) {
        case 0:
            if (step > 0) {
                left = black;
                right = green;
            }
            else if (step < 0) {
                left = green;
                right = black;
            }
            else {
                left = right = green;
            }
            break;
        case -ERANGE:
        case -ENOENT:
            if (dfplayer_step(dfplayer_get(0), 0)) {
                left = right = red;
            }
            else {
                if (step > 0) {
                    left = black;
                    right = red;
                }
                else {
                    left = red;
                    right = black;
                }
            }
            break;
        default:
            left = right = red;
    }


    for (uint16_t i = 0; i < neo.params.numof / 2; i++) {
        neopixel_set(&neo, i, right);
    }

    for (uint16_t i = neo.params.numof / 2; i < neo.params.numof; i++) {
        neopixel_set(&neo, i, left);
    }

    neopixel_write(&neo);

    xtimer_usleep(SELECT_DELAY_MS * US_PER_MS);
}

static void playback(bool paused)
{
    static uint16_t pos = 0;

    pos++;
    if (pos >= neo.params.numof) {
        pos = 0;
    }

    dfplayer_state_t state = DFPLAYER_STATE_STOPPED;
    if (dfplayer_get_state(dfplayer_get(0), &state)){
        puts("Error: Failed to get current state");
        return;
    }

    color_rgb_t front, back = black;
    if (paused) {
        front = blue;
        if (state == DFPLAYER_STATE_PLAYING) {
            if (dfplayer_pause(dfplayer_get(0))) {
                puts("Error: Failed to pause playback");
                return;
            }
        }
    }
    else {
        front = green;
        switch (state) {
            case DFPLAYER_STATE_PAUSED:
                if (dfplayer_play(dfplayer_get(0))) {
                    puts("Error: Failed to resume playback");
                    return;
                }
                break;
            case DFPLAYER_STATE_STOPPED:
                step(1);
                return;
            default:
                break;
        }
    }

    for (uint16_t i = 0; i < neo.params.numof; i++) {
        neopixel_set(&neo, i, (i == pos) ? front : back);
    }

    neopixel_write(&neo);
    xtimer_usleep(SELECT_DELAY_MS * US_PER_MS);
}

void * control_thread(void *_unused)
{
    (void)_unused;
    uint8_t volume = 15;

    if (neopixel_init(&neo, &neopixel_params[0])) {
        puts("Initializing NeoPixel driver failed, control thread gives up\n");
        return NULL;
    }

    if (mpu9150_init(&mpu, &mpu9150_params[0])) {
        puts("Initializing MPU9x50 driver failed, control thread gives up\n");
        return NULL;
    }

    if (dfplayer_set_volume(dfplayer_get(0), volume)) {
        puts("Error: Failed to set volume");
    }

    if (dfplayer_play_from_mp3(dfplayer_get(0), 1)) {
        puts("Error: Failed to start playback of first track");
    }

    while (1) {
        mpu9150_results_t accel;

        if (mpu9150_read_accel(&mpu, &accel)) {
            puts("Error: Failed to read from MPU9x50");
            continue;
        }

        if (accel.z_axis < THR_ACC_PAUSE) {
            playback(true);
        }
        else if (accel.x_axis < -THR_ACC_VOLUME) {
            if (volume < DFPLAYER_MAX_VOLUME) {
                set_volume(++volume);
            }
        }
        else if (accel.x_axis > THR_ACC_VOLUME) {
            if (volume > 0) {
                set_volume(--volume);
            }
        }
        else if (accel.y_axis < -THR_ACC_SELECT) {
            step(1);
        }
        else if (accel.y_axis > THR_ACC_SELECT) {
            step(-1);
        }
        else {
            playback(false);
        }
    }
}
