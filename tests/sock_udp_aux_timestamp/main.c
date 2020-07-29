/*
 * Copyright (C) 2020 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Test application for PTP timestamping through the sock API
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "fmt.h"
#include "net/sock/udp.h"
#include "shell.h"
#include "shell_commands.h"

#define MAIN_QUEUE_SIZE     (8)

#define PORT                12345
#define _QUOTE(x)           #x
#define QUOTE(x)            _QUOTE(x)

static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
char server_thread_stack[THREAD_STACKSIZE_DEFAULT];


static void *server_thread(void *arg)
{
    (void)arg;
    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
    sock_udp_t sock;
    local.port = PORT;
    if (sock_udp_create(&sock, &local, NULL, 0) < 0) {
        print_str("Error creating UDP sock\n");
        return NULL;
    }
    print_str("UDP echo server listening at port " QUOTE(PORT) "\n");
    while (1) {
        sock_udp_ep_t remote;
        ssize_t res;
        sock_udp_aux_rx_t rx_aux;
        sock_udp_aux_tx_t tx_aux;
        char buf[128];
        if (0 <= (res = sock_udp_recv_aux(&sock, buf, sizeof(buf),
                                          SOCK_NO_TIMEOUT, &remote, &rx_aux)))
            {
            print_str("Received a message at: ");
            if (rx_aux.flags & SOCK_AUX_HAS_TIMESTAMP) {
                print_u64_dec(rx_aux.timestamp);
                print_str(" ns\n");
            }
            else {
                print_str("No timestamp\n");
            }
            if (sock_udp_send_aux(&sock, buf, res, &remote, &tx_aux) < 0) {
                print_str("Error sending reply\n");
            }
            else {
                print_str("Sent echo at: ");
                if (tx_aux.flags & SOCK_AUX_HAS_TIMESTAMP) {
                    print_u64_dec(tx_aux.timestamp);
                    print_str(" ns\n");
                }
                else {
                    print_str("No timestamp\n");
                }
            }
        }
    }
    return 0;
}

int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    thread_create(server_thread_stack, sizeof(server_thread_stack),
                  THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
                  server_thread, NULL, "UDP echo server");

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
