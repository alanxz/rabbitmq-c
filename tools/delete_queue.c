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
  static int if_unused = 0;
  static int if_empty = 0;

  struct poptOption options[] = {
      INCLUDE_OPTIONS(connect_options),
      {"queue", 'q', POPT_ARG_STRING, &queue, 0, "the queue name to delete",
       "queue"},
      {"if-unused", 'u', POPT_ARG_VAL, &if_unused, 1,
       "do not delete unless queue is unused", NULL},
      {"if-empty", 'e', POPT_ARG_VAL, &if_empty, 1,
       "do not delete unless queue is empty", NULL},
      POPT_AUTOHELP{NULL, '\0', 0, NULL, 0, NULL, NULL}};

  process_all_options(argc, argv, options);

  if (queue == NULL || *queue == '\0') {
    fprintf(stderr, "queue name not specified\n");
    return 1;
  }

  conn = make_connection();
  {
    amqp_queue_delete_ok_t *reply =
        amqp_queue_delete(conn, 1, cstring_bytes(queue), if_unused, if_empty);
    if (reply == NULL) {
      die_rpc(amqp_get_rpc_reply(conn), "queue.delete");
    }
    printf("%u\n", reply->message_count);
  }
  close_connection(conn);
  return 0;
}
