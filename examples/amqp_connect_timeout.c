// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

#include <assert.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Winsock2.h>
#else
#include <sys/time.h>
#endif

#include "utils.h"

int main(int argc, char const *const *argv) {
  char const *hostname;
  int port;
  amqp_socket_t *socket;
  amqp_connection_state_t conn;
  struct timeval tval;
  struct timeval *tv;

  if (argc < 3) {
    fprintf(stderr,
            "Usage: amqp_connect_timeout host port [timeout_sec "
            "[timeout_usec=0]]\n");
    return 1;
  }

  if (argc > 3) {
    tv = &tval;

    tv->tv_sec = atoi(argv[3]);

    if (argc > 4) {
      tv->tv_usec = atoi(argv[4]);
    } else {
      tv->tv_usec = 0;
    }

  } else {
    tv = NULL;
  }

  hostname = argv[1];
  port = atoi(argv[2]);

  conn = amqp_new_connection();

  socket = amqp_tcp_socket_new(conn);

  if (!socket) {
    die("creating TCP socket");
  }

  die_on_error(amqp_socket_open_noblock(socket, hostname, port, tv),
               "opening TCP socket");

  die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                               "guest", "guest"),
                    "Logging in");

  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
                    "Closing connection");
  die_on_error(amqp_destroy_connection(conn), "Ending connection");

  printf("Done\n");
  return 0;
}
