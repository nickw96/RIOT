/*
 * Copyright (C) 2020 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief   Application for testing context switching triggered from IRQ
 *
 * @author  Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <stdatomic.h>
#include <stdio.h>

#include "thread.h"
#include "xtimer.h"
#include "mutex.h"

#define TEST_TIME (100UL * US_PER_MS)

static mutex_t mut = MUTEX_INIT_LOCKED;
static atomic_uint_least8_t signal = ATOMIC_VAR_INIT(0);
static char t2_stack[THREAD_STACKSIZE_DEFAULT];

static void _cb(void *unused)
{
    (void)unused;
    puts("[isr] unlocking mutex");
    mutex_unlock(&mut);
}

static void *t2_impl(void *unused)
{
    (void)unused;
    puts("[t2] trying to lock mutex");
    mutex_lock(&mut);
    puts("[t2] got mutex, setting signal");
    atomic_store(&signal, 1);
    mutex_lock(&mut);
    puts("[t2] this should have never been reached!");
    return NULL;
}

int main(void)
{
    puts("[main] starting thread t2");
    thread_create(t2_stack, sizeof(t2_stack),
                  THREAD_PRIORITY_MAIN - 1,
                  THREAD_CREATE_STACKTEST,
                  t2_impl, NULL, "t2");

    xtimer_t timer;
    timer.callback = _cb;
    puts("[main] starting timer to unlock mutex");
    xtimer_set(&timer, TEST_TIME);

    puts("[main] busy waiting for signal");
    while (!atomic_load(&signal)) { }
    puts("[main] completed");

    return 0;
}
