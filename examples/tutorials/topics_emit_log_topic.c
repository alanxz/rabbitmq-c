// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * RabbitMQ tutorial 5 ("Topics"): the log emitter.
 * https://www.rabbitmq.com/tutorials/tutorial-five-python
 *
 * Publishes a log message to the "topic_logs" topic exchange. The routing key
 * is a dot-separated list of words, e.g. "kern.critical" (facility.severity).
 * Queues bind with patterns where `*` matches exactly one word and `#` matches
 * zero or more words.
 *
 * Usage: topics_emit_log_topic [amqp-url] [routing-key] [message...]
 *   topics_emit_log_topic amqp://guest:guest@localhost:5672/%2F kern.critical \
 *     "A critical kernel error"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>

#include "tutorial_common.h"

#define EXCHANGE_NAME "topic_logs"

int main(int argc, char const *const *argv) {
  amqp_connection_state_t conn = NULL;
  char const *routing_key = argc > 2 ? argv[2] : "anonymous.info";
  char const *body = argc > 3 ? argv[3] : "Hello World!";
  int exit_code = EXIT_FAILURE;

  if (tutorial_connect(tutorial_broker_url(argc, argv), &conn) != TUTORIAL_OK) {
    return exit_code;
  }

  amqp_exchange_declare(
      conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(EXCHANGE_NAME),
      amqp_cstring_bytes("topic"), 0, /*durable*/ 1, 0, 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Declaring exchange") !=
      TUTORIAL_OK) {
    goto out;
  }

  if (tutorial_check_status(
          amqp_basic_publish(
              conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(EXCHANGE_NAME),
              amqp_cstring_bytes(routing_key),
              /*mandatory*/ 0, /*immediate*/ 0, NULL, amqp_cstring_bytes(body)),
          "Publishing") != TUTORIAL_OK) {
    goto out;
  }

  printf(" [x] Sent %s:'%s'\n", routing_key, body);
  exit_code = EXIT_SUCCESS;

out:
  tutorial_close(conn);
  return exit_code;
}
