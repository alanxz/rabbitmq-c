// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

int main(int argc, const char **argv) {
  amqp_connection_state_t conn;
  static char *queue = NULL;
  static int durable = 0;

  struct poptOption options[] = {
      INCLUDE_OPTIONS(connect_options),
      {"queue", 'q', POPT_ARG_STRING, &queue, 0,
       "the queue name to declare, or the empty string", "queue"},
      {"durable", 'd', POPT_ARG_VAL, &durable, 1, "declare a durable queue",
       NULL},
      POPT_AUTOHELP{NULL, '\0', 0, NULL, 0, NULL, NULL}};

  process_all_options(argc, argv, options);

  if (queue == NULL) {
    fprintf(stderr, "queue name not specified\n");
    return 1;
  }

  conn = make_connection();
  {
    amqp_queue_declare_ok_t *reply = amqp_queue_declare(
        conn, 1, cstring_bytes(queue), 0, durable, 0, 0, amqp_empty_table);
    if (reply == NULL) {
      die_rpc(amqp_get_rpc_reply(conn), "queue.declare");
    }

    printf("%.*s\n", (int)reply->queue.len, (char *)reply->queue.bytes);
  }
  close_connection(conn);
  return 0;
}
