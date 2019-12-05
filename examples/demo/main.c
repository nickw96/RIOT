#include <stdio.h>

#include "dfplayer.h"
#include "net/nanocoap_sock.h"
#include "shell.h"
#include "thread.h"
#include "ws281x.h"
#include "ws281x_params.h"
#include "xtimer.h"

#include "demo.h"

#define COAP_INBUF_SIZE     (256U)
#define COAP_QUEUE_SIZE     (8)

ws281x_t ws281x_dev;
atomic_int dfp_mode = ATOMIC_VAR_INIT(DFP_STOP_AT_END);

static msg_t coap_msg_queue[COAP_QUEUE_SIZE];
static char coap_thread_stack[THREAD_STACKSIZE_MAIN];
static char dfp_thread_stack[THREAD_STACKSIZE_DEFAULT];
static kernel_pid_t dfp_pid;

static void *coap_thread(void *arg)
{
    (void)arg;
    /* nanocoap_server uses gnrc sock which uses gnrc which needs a msg queue */
    msg_init_queue(coap_msg_queue, ARRAY_SIZE(coap_msg_queue));

    xtimer_sleep(3);

    /* initialize nanocoap server instance */
    uint8_t buf[COAP_INBUF_SIZE];
    sock_udp_ep_t local = { .port=COAP_PORT, .family=AF_INET6 };
    nanocoap_server(&local, buf, sizeof(buf));
    return NULL;
}

static void *dfp_thread(void *arg)
{
    dfplayer_t *dfp = arg;

    while (1) {
        thread_sleep();
        switch (atomic_load(&dfp_mode)) {
            default:
                break;
            case DFP_REPEAT:
                dfplayer_step(dfp, 0);
                break;
            case DFP_CONTINUOUS:
                dfplayer_next(dfp);
                break;
        }
    }

    return NULL;
}

static void dfp_done(dfplayer_source_t src, uint16_t track, void *data)
{
    (void)src;
    (void)track;
    (void)data;
    thread_wakeup(dfp_pid);
}

int main(void)
{
    if (ws281x_init(&ws281x_dev, &ws281x_params[0])) {
        puts("Failed to init WS281x");
    }

    thread_create(coap_thread_stack, sizeof(coap_thread_stack),
                  THREAD_PRIORITY_MAIN - 2, THREAD_CREATE_STACKTEST,
                  coap_thread, NULL, "coap");
    dfp_pid = thread_create(dfp_thread_stack, sizeof(dfp_thread_stack),
                            THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
                            dfp_thread, dfplayer_get(0), "dfp");

    dfplayer_set_callbacks(dfplayer_get(0), dfp_done, NULL, NULL);

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}
