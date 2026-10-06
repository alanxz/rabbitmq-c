// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * RabbitMQ tutorial 3 ("Publish/Subscribe"): the log emitter.
 * https://www.rabbitmq.com/tutorials/tutorial-three-python
 *
 * Publishes a log message to a fanout exchange, which copies it to every queue
 * bound to it. The producer never names a queue; it only knows the exchange.
 *
 * Usage: publish_subscribe_emit_log [amqp-url] [message...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>

#include "tutorial_common.h"

#define EXCHANGE_NAME "logs"

int main(int argc, char const *const *argv) {
  amqp_connection_state_t conn = NULL;
  char const *body = argc > 2 ? argv[2] : "info: Hello World!";
  int exit_code = EXIT_FAILURE;

  if (tutorial_connect(tutorial_broker_url(argc, argv), &conn) != TUTORIAL_OK) {
    return exit_code;
  }

  /* Fanout exchanges ignore the routing key. Declaring is idempotent, and both
   * sides declare it so start-up order does not matter. */
  amqp_exchange_declare(
      conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(EXCHANGE_NAME),
      amqp_cstring_bytes("fanout"), /*passive*/ 0,
      /*durable*/ 1, /*auto_delete*/ 0, /*internal*/ 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Declaring exchange") !=
      TUTORIAL_OK) {
    goto out;
  }

  /* Logs are disposable, so the message is transient (no delivery_mode) and
   * mandatory is off: with no subscribers a fanout message is simply dropped,
   * which is the intended behaviour here. */
  if (tutorial_check_status(
          amqp_basic_publish(
              conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(EXCHANGE_NAME),
              amqp_empty_bytes,
              /*mandatory*/ 0, /*immediate*/ 0, NULL, amqp_cstring_bytes(body)),
          "Publishing") != TUTORIAL_OK) {
    goto out;
  }

  printf(" [x] Sent '%s'\n", body);
  exit_code = EXIT_SUCCESS;

out:
  tutorial_close(conn);
  return exit_code;
}
