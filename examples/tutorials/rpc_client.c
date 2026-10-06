// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * RabbitMQ tutorial 6 ("RPC"): the client.
 * https://www.rabbitmq.com/tutorials/tutorial-six-python
 *
 * Sends a request to "rpc_queue" and waits for the matching reply, with a
 * deadline so a dead or missing server cannot hang the caller forever.
 *
 * Usage: rpc_client [amqp-url] [n]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <rabbitmq-c/amqp.h>

#include "tutorial_common.h"

#define QUEUE_NAME "rpc_queue"
#define REPLY_TIMEOUT_SECONDS 10
#define POLL_INTERVAL_SECONDS 1
/* The broker discards requests that sit unprocessed for this long (ms), so a
 * request we have already given up on does not run later. */
#define REQUEST_TTL_MS "10000"

int main(int argc, char const *const *argv) {
  amqp_connection_state_t conn = NULL;
  amqp_bytes_t reply_queue = amqp_empty_bytes;
  char const *request = argc > 2 ? argv[2] : "30";
  char correlation_id[64];
  time_t deadline;
  int exit_code = EXIT_FAILURE;

  if (tutorial_connect(tutorial_broker_url(argc, argv), &conn) != TUTORIAL_OK) {
    return exit_code;
  }

  /* Private, broker-named queue for replies. */
  {
    amqp_queue_declare_ok_t *declared = amqp_queue_declare(
        conn, TUTORIAL_CHANNEL, amqp_empty_bytes, 0, /*durable*/ 0,
        /*exclusive*/ 1, /*auto_delete*/ 1, amqp_empty_table);
    if (tutorial_check_reply(amqp_get_rpc_reply(conn),
                             "Declaring reply queue") != TUTORIAL_OK) {
      goto out;
    }
    reply_queue = amqp_bytes_malloc_dup(declared->queue);
    if (reply_queue.bytes == NULL) {
      fprintf(stderr, "Out of memory\n");
      goto out;
    }
  }

  /* Start consuming replies before sending the request so none can be missed.
   */
  amqp_basic_consume(conn, TUTORIAL_CHANNEL, reply_queue, amqp_empty_bytes, 0,
                     /*no_ack*/ 1, /*exclusive*/ 1, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Starting consumer") !=
      TUTORIAL_OK) {
    goto out;
  }

  /* Unique per request; lets us ignore stale or foreign replies. */
  snprintf(correlation_id, sizeof(correlation_id), "%lu-%lu",
           (unsigned long)time(NULL), (unsigned long)clock());

  {
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_REPLY_TO_FLAG |
                   AMQP_BASIC_CORRELATION_ID_FLAG | AMQP_BASIC_EXPIRATION_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.reply_to = reply_queue;
    props.correlation_id = amqp_cstring_bytes(correlation_id);
    props.expiration = amqp_cstring_bytes(REQUEST_TTL_MS);

    /* mandatory=1: if rpc_queue does not exist the broker returns the request
     * (reported by tutorial_wait_for_message below) instead of dropping it. */
    if (tutorial_check_status(
            amqp_basic_publish(conn, TUTORIAL_CHANNEL, amqp_empty_bytes,
                               amqp_cstring_bytes(QUEUE_NAME), /*mandatory*/ 1,
                               0, &props, amqp_cstring_bytes(request)),
            "Publishing request") != TUTORIAL_OK) {
      goto out;
    }
  }

  printf(" [x] Requesting fib(%s)\n", request);

  deadline = time(NULL) + REPLY_TIMEOUT_SECONDS;
  while (time(NULL) < deadline) {
    amqp_envelope_t envelope;
    amqp_basic_properties_t const *p;
    int matches;

    switch (tutorial_wait_for_message(conn, &envelope, POLL_INTERVAL_SECONDS)) {
      case TUTORIAL_WAIT_IDLE:
        continue;
      case TUTORIAL_WAIT_FAILED:
        goto out;
      case TUTORIAL_WAIT_MESSAGE:
        break;
    }

    p = &envelope.message.properties;
    matches = (p->_flags & AMQP_BASIC_CORRELATION_ID_FLAG) &&
              p->correlation_id.len == strlen(correlation_id) &&
              memcmp(p->correlation_id.bytes, correlation_id,
                     p->correlation_id.len) == 0;
    if (matches) {
      printf(" [.] Got %.*s\n", (int)envelope.message.body.len,
             (char *)envelope.message.body.bytes);
      exit_code = EXIT_SUCCESS;
    }
    amqp_destroy_envelope(&envelope);
    if (matches) {
      goto out;
    }
    /* Anything else is a reply to some earlier request: ignore it. */
  }

  fprintf(stderr, "Timed out after %d seconds waiting for a reply\n",
          REPLY_TIMEOUT_SECONDS);

out:
  amqp_bytes_free(reply_queue);
  tutorial_close(conn);
  return exit_code;
}
