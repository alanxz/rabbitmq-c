// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * RabbitMQ tutorial 1 ("Hello World"): the producer.
 * https://www.rabbitmq.com/tutorials/tutorial-one-python
 *
 * Declares a durable queue called "hello" and publishes one message to it via
 * the default exchange.
 *
 * Usage: hello_world_send [amqp-url] [message]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>

#include "tutorial_common.h"

#define QUEUE_NAME "hello"

int main(int argc, char const *const *argv) {
  amqp_connection_state_t conn = NULL;
  const char *body = argc > 2 ? argv[2] : "Hello World!";
  int exit_code = EXIT_FAILURE;
  int status;

  if (tutorial_connect(tutorial_broker_url(argc, argv), &conn) != TUTORIAL_OK) {
    return exit_code;
  }

  /* Declaring is idempotent: it creates the queue if missing and is a no-op if
   * it already exists with the same arguments. A durable queue survives a
   * broker restart. */
  amqp_queue_declare(conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(QUEUE_NAME),
                     /*passive*/ 0, /*durable*/ 1, /*exclusive*/ 0,
                     /*auto_delete*/ 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Declaring queue") !=
      TUTORIAL_OK) {
    goto out;
  }

  {
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = AMQP_DELIVERY_PERSISTENT;

    /* The default exchange ("") routes by queue name. mandatory=1 asks the
     * broker to return the message if it could not be routed to any queue. */
    status = amqp_basic_publish(conn, TUTORIAL_CHANNEL, amqp_empty_bytes,
                                amqp_cstring_bytes(QUEUE_NAME),
                                /*mandatory*/ 1, /*immediate*/ 0, &props,
                                amqp_cstring_bytes(body));
    if (tutorial_check_status(status, "Publishing") != TUTORIAL_OK) {
      goto out;
    }
  }

  printf(" [x] Sent '%s'\n", body);
  exit_code = EXIT_SUCCESS;

out:
  tutorial_close(conn);
  return exit_code;
}
