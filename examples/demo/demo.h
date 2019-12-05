#ifndef DEMO_H
#define DEMO_H

#include <stdint.h>
#include <stdatomic.h>

#include "mpu9x50.h"
#include "ws281x.h"

enum {
    DFP_STOP_AT_END,
    DFP_REPEAT,
    DFP_CONTINUOUS,
};

extern mpu9x50_t mpu9x50_devs[1];
extern ws281x_t ws281x_dev;
extern atomic_int dfp_mode;

#endif
