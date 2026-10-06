// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * RabbitMQ tutorial 1 ("Hello World"): the consumer.
 * https://www.rabbitmq.com/tutorials/tutorial-one-python
 *
 * Declares the "hello" queue, subscribes to it and prints every message until
 * interrupted with Ctrl-C. Messages are acknowledged only after they have been
 * handled, so nothing is lost if this process dies mid-message.
 *
 * Usage: hello_world_receive [amqp-url]
 */

#include <stdio.h>
#include <stdlib.h>

#include <rabbitmq-c/amqp.h>

#include "tutorial_common.h"

#define QUEUE_NAME "hello"

/* How long to block waiting for a message before checking whether we were asked
 * to stop. */
#define POLL_INTERVAL_SECONDS 1

int main(int argc, char const *const *argv) {
  amqp_connection_state_t conn = NULL;
  int exit_code = EXIT_FAILURE;

  tutorial_install_signal_handlers();

  if (tutorial_connect(tutorial_broker_url(argc, argv), &conn) != TUTORIAL_OK) {
    return exit_code;
  }

  amqp_queue_declare(conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(QUEUE_NAME), 0,
                     /*durable*/ 1, 0, 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Declaring queue") !=
      TUTORIAL_OK) {
    goto out;
  }

  /* no_ack=0: the broker keeps each message until we ack it. */
  amqp_basic_consume(conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(QUEUE_NAME),
                     amqp_empty_bytes, /*no_local*/ 0, /*no_ack*/ 0,
                     /*exclusive*/ 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Starting consumer") !=
      TUTORIAL_OK) {
    goto out;
  }

  printf(" [*] Waiting for messages. To exit press CTRL+C\n");

  while (!tutorial_interrupted()) {
    amqp_envelope_t envelope;
    amqp_rpc_reply_t reply;
    struct timeval timeout = {POLL_INTERVAL_SECONDS, 0};

    /* Returns memory held by the previous message back to the library. */
    amqp_maybe_release_buffers(conn);

    reply = amqp_consume_message(conn, &envelope, &timeout, 0);

    if (reply.reply_type == AMQP_RESPONSE_NORMAL) {
      printf(" [x] Received '%.*s'\n", (int)envelope.message.body.len,
             (char *)envelope.message.body.bytes);

      /* Acknowledge only after the work is done. If this fails the
       * connection is unusable, so stop. */
      if (tutorial_check_status(
              amqp_basic_ack(conn, envelope.channel, envelope.delivery_tag,
                             /*multiple*/ 0),
              "Acknowledging message") != TUTORIAL_OK) {
        amqp_destroy_envelope(&envelope);
        goto out;
      }
      amqp_destroy_envelope(&envelope);
      continue;
    }

    if (reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
      if (reply.library_error == AMQP_STATUS_TIMEOUT) {
        continue; /* nothing arrived, loop around and check for Ctrl-C */
      }
      if (reply.library_error == AMQP_STATUS_UNEXPECTED_STATE) {
        if (tutorial_handle_unexpected_frame(conn) == TUTORIAL_OK) {
          continue;
        }
        goto out;
      }
    }

    /* Anything else (socket closed, heartbeat timeout, ...) is fatal for this
     * connection. A long lived service would reconnect here with backoff. */
    tutorial_check_reply(reply, "Consuming");
    goto out;
  }

  printf(" [*] Interrupted, shutting down\n");
  exit_code = EXIT_SUCCESS;

out:
  tutorial_close(conn);
  return exit_code;
}
