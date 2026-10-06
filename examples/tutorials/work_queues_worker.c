// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * RabbitMQ tutorial 2 ("Work queues"): the worker.
 * https://www.rabbitmq.com/tutorials/tutorial-two-python
 *
 * Run several workers against the same queue and the broker spreads tasks
 * between them. Each dot in a task body costs one second of "work".
 *
 * Reliability measures demonstrated:
 *  - manual acknowledgements, so a task whose worker dies is redelivered
 *  - prefetch of 1, so a busy worker is not handed more work while another
 *    worker sits idle (fair dispatch)
 *  - a durable queue, so tasks survive a broker restart
 *  - nack + requeue when a task cannot be processed
 *
 * Usage: work_queues_worker [amqp-url]
 */

#include <stdio.h>
#include <stdlib.h>

#include <rabbitmq-c/amqp.h>

#include "tutorial_common.h"

#define QUEUE_NAME "task_queue"
#define POLL_INTERVAL_SECONDS 1

/* Simulated work. Returns 0 on success, -1 if the worker was interrupted
 * before finishing, in which case the task must not be acknowledged. */
static int do_work(amqp_bytes_t body) {
  size_t i;
  for (i = 0; i < body.len; i++) {
    if (((char *)body.bytes)[i] == '.') {
      /* Sleep in short slices so Ctrl-C is honoured promptly. */
      int slice;
      for (slice = 0; slice < 10; slice++) {
        if (tutorial_interrupted()) {
          return -1;
        }
        tutorial_sleep_ms(100);
      }
    }
  }
  return 0;
}

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

  /* Fair dispatch: at most one unacknowledged message per consumer. Must be
   * set before basic.consume. */
  amqp_basic_qos(conn, TUTORIAL_CHANNEL, /*prefetch_size*/ 0,
                 /*prefetch_count*/ 1, /*global*/ 0);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Setting prefetch") !=
      TUTORIAL_OK) {
    goto out;
  }

  amqp_basic_consume(conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(QUEUE_NAME),
                     amqp_empty_bytes, 0, /*no_ack*/ 0, 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Starting consumer") !=
      TUTORIAL_OK) {
    goto out;
  }

  printf(" [*] Waiting for messages. To exit press CTRL+C\n");

  while (!tutorial_interrupted()) {
    amqp_envelope_t envelope;
    int status;

    switch (tutorial_wait_for_message(conn, &envelope, POLL_INTERVAL_SECONDS)) {
      case TUTORIAL_WAIT_IDLE:
        continue;
      case TUTORIAL_WAIT_FAILED:
        goto out;
      case TUTORIAL_WAIT_MESSAGE:
        break;
    }

    printf(" [x] Received '%.*s'\n", (int)envelope.message.body.len,
           (char *)envelope.message.body.bytes);

    if (do_work(envelope.message.body) == 0) {
      printf(" [x] Done\n");
      status = amqp_basic_ack(conn, envelope.channel, envelope.delivery_tag, 0);
    } else {
      /* Interrupted mid-task: hand it back so another worker can take it. */
      printf(" [!] Interrupted, returning task to the queue\n");
      status = amqp_basic_nack(conn, envelope.channel, envelope.delivery_tag,
                               /*multiple*/ 0, /*requeue*/ 1);
    }
    amqp_destroy_envelope(&envelope);

    if (tutorial_check_status(status, "Acknowledging task") != TUTORIAL_OK) {
      goto out;
    }
  }

  exit_code = EXIT_SUCCESS;

out:
  tutorial_close(conn);
  return exit_code;
}
