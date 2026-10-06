// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * RabbitMQ tutorial 5 ("Topics"): the pattern-matching log receiver.
 * https://www.rabbitmq.com/tutorials/tutorial-five-python
 *
 * Binds a private queue to the "topic_logs" exchange once per binding pattern
 * and prints what arrives. `*` matches exactly one word, `#` zero or more.
 *
 * Usage: topics_receive_logs_topic amqp-url binding-key [binding-key...]
 *   topics_receive_logs_topic amqp://guest:guest@localhost:5672/%2F warning
 * error
 */

#include <stdio.h>
#include <stdlib.h>

#include <rabbitmq-c/amqp.h>

#include "tutorial_common.h"

#define EXCHANGE_NAME "topic_logs"
#define POLL_INTERVAL_SECONDS 1

int main(int argc, char const *const *argv) {
  amqp_connection_state_t conn = NULL;
  amqp_bytes_t queue = amqp_empty_bytes;
  int exit_code = EXIT_FAILURE;
  int i;

  /* The URL is mandatory here so the binding keys are unambiguous. */
  if (argc < 3) {
    fprintf(stderr, "Usage: %s amqp-url binding-key [binding-key...]\n",
            argv[0]);
    return exit_code;
  }

  tutorial_install_signal_handlers();

  if (tutorial_connect(argv[1], &conn) != TUTORIAL_OK) {
    return exit_code;
  }

  amqp_exchange_declare(
      conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(EXCHANGE_NAME),
      amqp_cstring_bytes("topic"), 0, /*durable*/ 1, 0, 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Declaring exchange") !=
      TUTORIAL_OK) {
    goto out;
  }

  {
    amqp_queue_declare_ok_t *declared = amqp_queue_declare(
        conn, TUTORIAL_CHANNEL, amqp_empty_bytes, 0, /*durable*/ 0,
        /*exclusive*/ 1, /*auto_delete*/ 1, amqp_empty_table);
    if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Declaring queue") !=
        TUTORIAL_OK) {
      goto out;
    }
    queue = amqp_bytes_malloc_dup(declared->queue);
    if (queue.bytes == NULL) {
      fprintf(stderr, "Out of memory\n");
      goto out;
    }
  }

  for (i = 2; i < argc; i++) {
    amqp_queue_bind(conn, TUTORIAL_CHANNEL, queue,
                    amqp_cstring_bytes(EXCHANGE_NAME),
                    amqp_cstring_bytes(argv[i]), amqp_empty_table);
    if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Binding queue") !=
        TUTORIAL_OK) {
      goto out;
    }
  }

  amqp_basic_consume(conn, TUTORIAL_CHANNEL, queue, amqp_empty_bytes, 0,
                     /*no_ack*/ 1, 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Starting consumer") !=
      TUTORIAL_OK) {
    goto out;
  }

  printf(" [*] Waiting for logs. To exit press CTRL+C\n");

  while (!tutorial_interrupted()) {
    amqp_envelope_t envelope;

    switch (tutorial_wait_for_message(conn, &envelope, POLL_INTERVAL_SECONDS)) {
      case TUTORIAL_WAIT_IDLE:
        continue;
      case TUTORIAL_WAIT_FAILED:
        goto out;
      case TUTORIAL_WAIT_MESSAGE:
        break;
    }

    printf(" [x] %.*s:'%.*s'\n", (int)envelope.routing_key.len,
           (char *)envelope.routing_key.bytes, (int)envelope.message.body.len,
           (char *)envelope.message.body.bytes);
    amqp_destroy_envelope(&envelope);
  }

  exit_code = EXIT_SUCCESS;

out:
  amqp_bytes_free(queue);
  tutorial_close(conn);
  return exit_code;
}
