// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * RabbitMQ tutorial 6 ("RPC"): the server.
 * https://www.rabbitmq.com/tutorials/tutorial-six-python
 *
 * Consumes requests from "rpc_queue". Each request body is a decimal number n;
 * the reply body is fib(n), or "error: <reason>" if the request is invalid.
 * Replies are published to the queue named in the request's reply_to property
 * and carry the request's correlation_id so the client can match them up.
 *
 * Usage: rpc_server [amqp-url]
 */

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>

#include "tutorial_common.h"

#define QUEUE_NAME "rpc_queue"
#define POLL_INTERVAL_SECONDS 1
/* fib(93) is the first value that overflows uint64_t. */
#define MAX_FIB_INPUT 92
#define MAX_REQUEST_LEN 16

static uint64_t fib(unsigned int n) {
  uint64_t a = 0, b = 1;
  unsigned int i;
  for (i = 0; i < n; i++) {
    uint64_t next = a + b;
    a = b;
    b = next;
  }
  return a;
}

/*
 * Turns a request body into a reply string. Never trust request contents: the
 * body is not NUL terminated, may be any length, and may not be a number.
 */
static void make_reply(amqp_bytes_t body, char *out, size_t out_len) {
  char text[MAX_REQUEST_LEN + 1];
  char *end;
  unsigned long n;

  if (body.len == 0 || body.len > MAX_REQUEST_LEN) {
    snprintf(out, out_len, "error: request must be 1-%d characters",
             MAX_REQUEST_LEN);
    return;
  }
  memcpy(text, body.bytes, body.len);
  text[body.len] = '\0';

  errno = 0;
  n = strtoul(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0') {
    snprintf(out, out_len, "error: '%s' is not a number", text);
    return;
  }
  if (n > MAX_FIB_INPUT) {
    snprintf(out, out_len, "error: n must be at most %d", MAX_FIB_INPUT);
    return;
  }
  snprintf(out, out_len, "%" PRIu64, fib((unsigned int)n));
}

int main(int argc, char const *const *argv) {
  amqp_connection_state_t conn = NULL;
  int exit_code = EXIT_FAILURE;

  tutorial_install_signal_handlers();

  if (tutorial_connect(tutorial_broker_url(argc, argv), &conn) != TUTORIAL_OK) {
    return exit_code;
  }

  amqp_queue_declare(conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(QUEUE_NAME), 0,
                     /*durable*/ 0, 0, 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Declaring queue") !=
      TUTORIAL_OK) {
    goto out;
  }

  /* Spread load fairly if several servers run. */
  amqp_basic_qos(conn, TUTORIAL_CHANNEL, 0, /*prefetch_count*/ 1, 0);
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

  printf(" [x] Awaiting RPC requests\n");

  while (!tutorial_interrupted()) {
    amqp_envelope_t envelope;
    amqp_basic_properties_t const *request_props;
    char reply[96];
    int status;

    switch (tutorial_wait_for_message(conn, &envelope, POLL_INTERVAL_SECONDS)) {
      case TUTORIAL_WAIT_IDLE:
        continue;
      case TUTORIAL_WAIT_FAILED:
        goto out;
      case TUTORIAL_WAIT_MESSAGE:
        break;
    }

    request_props = &envelope.message.properties;
    make_reply(envelope.message.body, reply, sizeof(reply));
    printf(" [.] request '%.*s' -> %s\n", (int)envelope.message.body.len,
           (char *)envelope.message.body.bytes, reply);

    if (!(request_props->_flags & AMQP_BASIC_REPLY_TO_FLAG) ||
        request_props->reply_to.len == 0) {
      /* Nowhere to send a reply. Redelivering would not help, so drop it. */
      fprintf(stderr, "Request has no reply_to, discarding\n");
    } else {
      amqp_basic_properties_t props;
      props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG;
      props.content_type = amqp_cstring_bytes("text/plain");
      if (request_props->_flags & AMQP_BASIC_CORRELATION_ID_FLAG) {
        props._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
        props.correlation_id = request_props->correlation_id;
      }

      /* Publish via the default exchange to the client's private queue.
       * mandatory=1 so a client that has already gone away is reported (as a
       * basic.return, handled by tutorial_wait_for_message) rather than
       * silently ignored. A vanished client is not a server failure. */
      status = amqp_basic_publish(conn, TUTORIAL_CHANNEL, amqp_empty_bytes,
                                  request_props->reply_to, /*mandatory*/ 1, 0,
                                  &props, amqp_cstring_bytes(reply));
      if (tutorial_check_status(status, "Publishing reply") != TUTORIAL_OK) {
        amqp_destroy_envelope(&envelope);
        goto out;
      }
    }

    /* Ack only after the reply is on its way. */
    status = amqp_basic_ack(conn, envelope.channel, envelope.delivery_tag, 0);
    amqp_destroy_envelope(&envelope);
    if (tutorial_check_status(status, "Acknowledging request") != TUTORIAL_OK) {
      goto out;
    }
  }

  exit_code = EXIT_SUCCESS;

out:
  tutorial_close(conn);
  return exit_code;
}
