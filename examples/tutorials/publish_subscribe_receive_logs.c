// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * RabbitMQ tutorial 3 ("Publish/Subscribe"): the log receiver.
 * https://www.rabbitmq.com/tutorials/tutorial-three-python
 *
 * Creates a private, broker-named queue, binds it to the "logs" fanout exchange
 * and prints everything that arrives. Start several receivers and each one gets
 * every message. The queue is exclusive, so it disappears with the connection.
 *
 * Usage: publish_subscribe_receive_logs [amqp-url]
 */

#include <stdio.h>
#include <stdlib.h>

#include <rabbitmq-c/amqp.h>

#include "tutorial_common.h"

#define EXCHANGE_NAME "logs"
#define POLL_INTERVAL_SECONDS 1

int main(int argc, char const *const *argv) {
  amqp_connection_state_t conn = NULL;
  amqp_bytes_t queue = amqp_empty_bytes;
  int exit_code = EXIT_FAILURE;

  tutorial_install_signal_handlers();

  if (tutorial_connect(tutorial_broker_url(argc, argv), &conn) != TUTORIAL_OK) {
    return exit_code;
  }

  amqp_exchange_declare(
      conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(EXCHANGE_NAME),
      amqp_cstring_bytes("fanout"), 0, /*durable*/ 1, 0, 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Declaring exchange") !=
      TUTORIAL_OK) {
    goto out;
  }

  /* An empty name asks the broker to generate one. exclusive=1 ties the queue
   * to this connection; auto_delete=1 removes it when the consumer goes. */
  {
    amqp_queue_declare_ok_t *declared = amqp_queue_declare(
        conn, TUTORIAL_CHANNEL, amqp_empty_bytes, 0, /*durable*/ 0,
        /*exclusive*/ 1, /*auto_delete*/ 1, amqp_empty_table);
    if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Declaring queue") !=
        TUTORIAL_OK) {
      goto out;
    }
    /* `declared` points into library-owned memory that the next call on this
     * connection invalidates, so take our own copy of the generated name. */
    queue = amqp_bytes_malloc_dup(declared->queue);
    if (queue.bytes == NULL) {
      fprintf(stderr, "Out of memory\n");
      goto out;
    }
  }

  /* Routing key is ignored by fanout exchanges. */
  amqp_queue_bind(conn, TUTORIAL_CHANNEL, queue,
                  amqp_cstring_bytes(EXCHANGE_NAME), amqp_empty_bytes,
                  amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Binding queue") !=
      TUTORIAL_OK) {
    goto out;
  }

  /* Log lines are disposable, so auto-ack (no_ack=1) is acceptable here. For
   * anything you can't afford to lose use manual acks as in the earlier
   * examples. */
  amqp_basic_consume(conn, TUTORIAL_CHANNEL, queue, amqp_empty_bytes, 0,
                     /*no_ack*/ 1, /*exclusive*/ 0, amqp_empty_table);
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

    printf(" [x] %.*s\n", (int)envelope.message.body.len,
           (char *)envelope.message.body.bytes);
    amqp_destroy_envelope(&envelope);
  }

  exit_code = EXIT_SUCCESS;

out:
  amqp_bytes_free(queue);
  tutorial_close(conn);
  return exit_code;
}
