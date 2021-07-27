// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <stdio.h>

#include "common.h"

static int do_get(amqp_connection_state_t conn, char *queue) {
  amqp_rpc_reply_t r = amqp_basic_get(conn, 1, cstring_bytes(queue), 1);
  die_rpc(r, "basic.get");

  if (r.reply.id == AMQP_BASIC_GET_EMPTY_METHOD) {
    return 0;
  }

  copy_body(conn, 1);
  return 1;
}

int main(int argc, const char **argv) {
  amqp_connection_state_t conn;
  static char *queue = NULL;
  int got_something;

  struct poptOption options[] = {
      INCLUDE_OPTIONS(connect_options),
      {"queue", 'q', POPT_ARG_STRING, &queue, 0, "the queue to consume from",
       "queue"},
      POPT_AUTOHELP{NULL, '\0', 0, NULL, 0, NULL, NULL}};

  process_all_options(argc, argv, options);

  if (!queue) {
    fprintf(stderr, "queue not specified\n");
    return 1;
  }

  conn = make_connection();
  got_something = do_get(conn, queue);
  close_connection(conn);
  return got_something ? 0 : 2;
}
