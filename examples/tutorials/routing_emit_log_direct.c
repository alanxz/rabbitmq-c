// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * RabbitMQ tutorial 4 ("Routing"): the log emitter.
 * https://www.rabbitmq.com/tutorials/tutorial-four-python
 *
 * Publishes a log message to the "direct_logs" direct exchange using the
 * severity as the routing key. Only queues bound with that exact key receive
 * it.
 *
 * Usage: routing_emit_log_direct [amqp-url] [severity] [message...]
 *   routing_emit_log_direct amqp://guest:guest@localhost:5672/%2F error "Boom"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>

#include "tutorial_common.h"

#define EXCHANGE_NAME "direct_logs"

int main(int argc, char const *const *argv) {
  amqp_connection_state_t conn = NULL;
  char const *severity = argc > 2 ? argv[2] : "info";
  char const *body = argc > 3 ? argv[3] : "Hello World!";
  int exit_code = EXIT_FAILURE;

  if (tutorial_connect(tutorial_broker_url(argc, argv), &conn) != TUTORIAL_OK) {
    return exit_code;
  }

  amqp_exchange_declare(
      conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(EXCHANGE_NAME),
      amqp_cstring_bytes("direct"), 0, /*durable*/ 1, 0, 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Declaring exchange") !=
      TUTORIAL_OK) {
    goto out;
  }

  if (tutorial_check_status(
          amqp_basic_publish(
              conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(EXCHANGE_NAME),
              amqp_cstring_bytes(severity),
              /*mandatory*/ 0, /*immediate*/ 0, NULL, amqp_cstring_bytes(body)),
          "Publishing") != TUTORIAL_OK) {
    goto out;
  }

  printf(" [x] Sent %s:'%s'\n", severity, body);
  exit_code = EXIT_SUCCESS;

out:
  tutorial_close(conn);
  return exit_code;
}
