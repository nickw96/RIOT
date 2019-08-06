/*
 * Copyright 2019 Marian Buschsieweke
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Test application for GPIO Advanced Bitbaning Capabilities (ABC)
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <inttypes.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "irq.h"
#include "periph/gpio_abc.h"
#include "shell.h"
#include "xtimer.h"

#define N_LOOPS                     (100U)

static int gpio_pin(int argc, char **argv);
static int gpio_abc(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "pin", "Select the pin to test", gpio_pin },
    { "abc", "toggles the GPIO 100 times with the given nano-second delay", gpio_abc },
    { NULL, NULL, NULL }
};

static _Atomic gpio_t _gpio = GPIO_UNDEF;
static atomic_int _delay = -1;
char toggler_stack[THREAD_STACKSIZE_DEFAULT];

static int gpio_pin(int argc, char **argv)
{
    int port, pin;
    gpio_t gpio;

    if (argc != 3) {
        printf("Usage: %s <port> <pin>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if ((*argv[1] >= 'A') && (*argv[1] <= 'Z')) {
        port = (int)*argv[1] - (int)'A';
    }
    else if ((*argv[1] >= 'a') && (*argv[1] <= 'z')) {
        port = (int)*argv[1] - (int)'a';
    }
    else {
        port = atoi(argv[1]);
    }

    pin = atoi(argv[2]);

    gpio = GPIO_PIN(port, pin);

    if (gpio_init(gpio, GPIO_OUT)) {
        printf("Error to initialize P%c%d / P%d.%d\n",
               (char)('A' + port), pin, port, pin);
        return EXIT_FAILURE;
    }

    gpio_clear(gpio);
    atomic_store(&_gpio, gpio);

    return EXIT_SUCCESS;
}

static int gpio_abc(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: %s <duration (in ns)>\n", argv[0]);
        return EXIT_FAILURE;
    }

    gpio_t gpio = atomic_load(&_gpio);
    if (gpio == GPIO_UNDEF) {
        puts("Run command \"pin\" first to select the pin to toggle");
        return EXIT_FAILURE;
    }

    uint16_t duration = (uint16_t)atoi(argv[1]);
    int delay = gpio_abc_delay(duration);

    if (delay == -1) {
        printf("A pulse length of %" PRIu16 " is too short for your board\n",
               duration);
        return EXIT_FAILURE;
    }

    printf("Toggling now with pulse lengths of %" PRIu16 "ns (delay = %d)\n",
           duration, delay);
    printf("Expected pulse length: %uns\n",
           (unsigned)(GPIO_ABC_OVERHEAD_NS + (1000000000ULL * GPIO_ABC_LOOP_CYCLES * (uint64_t)delay) / CLOCK_CORECLOCK));
    atomic_store(&_delay, delay);

    return EXIT_SUCCESS;
}

static void *toggler(void *arg)
{
    (void)arg;
    while (1) {
        gpio_t gpio = atomic_load(&_gpio);
        int delay = atomic_load(&_delay);

        if ((gpio != GPIO_UNDEF) && (delay != -1)) {
            for (unsigned i = 0; i < N_LOOPS; i++) {
                gpio_set_for(gpio, delay);
                gpio_clear_for(gpio, delay);
            }
        }
        xtimer_usleep(100);
    }

    return NULL;
}

int main(void)
{
    puts(
        "GPIO Advanced Bitbanging Capabilities (ABC) Test\n"
        "================================================\n"
        "\n"
        "Prerequisites\n"
        "-------------\n"
        "\n"
        "- A digital oscilloscope or a logic analyzer with a sample rate of\n"
        "  at least 20 MHz (50ns resolution)\n"
        "- A board with GPIO ABC support\n"
        "\n"
        "Testing\n"
        "-------\n"
        "\n"
        "1. Connect the scope or the logic analyzer to your favourite GPIO\n"
        "2. Setup that pin using the \"pin\" command\n"
        "3. Run \"abc\" with durations of your choosing and verify that the\n"
        "   durations match the one you specified\n"
        "\n"
        "Board Properties\n"
        "-------\n"
    );

    printf("Shortest pulse:   %uns\n", (unsigned)GPIO_ABC_MIN_PULSE_LEN);
    printf("Accuracy(*):      %uns\n", (unsigned)GPIO_ABC_ACCURACY_NS);
    printf("CPU Clock:        %u %03u %03uHz\n",
           (unsigned)(CLOCK_CORECLOCK / 1000000U),
           (unsigned)((CLOCK_CORECLOCK / 1000U) % 1000U),
           (unsigned)(CLOCK_CORECLOCK % 1000000U));
    printf("CPU Cycle Length: %uns\n",
           (unsigned)((1000000000ULL + CLOCK_CORECLOCK / 2) / CLOCK_CORECLOCK));
    puts("\n(*) Worst case accuracy if GPIO ABC parameters are perfect");

    /* start toggling thread */
    thread_create(toggler_stack, sizeof(toggler_stack),
                  THREAD_PRIORITY_MAIN + 1, THREAD_CREATE_STACKTEST,
                  toggler, NULL, "toggler");

    /* start the shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
