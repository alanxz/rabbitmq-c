// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * RabbitMQ tutorial 2 ("Work queues"): the task producer.
 * https://www.rabbitmq.com/tutorials/tutorial-two-python
 *
 * Publishes one task to the durable "task_queue" queue. Every dot in the
 * message stands for one second of simulated work in the worker.
 *
 * Usage: work_queues_new_task [amqp-url] [message...]
 *   work_queues_new_task amqp://guest:guest@localhost:5672/%2F First message...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>

#include "tutorial_common.h"

#define QUEUE_NAME "task_queue"

/* Joins argv[first..argc) with spaces into a malloc()ed string. */
static char *join_args(int first, int argc, char const *const *argv) {
  size_t len = 1;
  char *out;
  int i;

  for (i = first; i < argc; i++) {
    len += strlen(argv[i]) + 1;
  }
  out = malloc(len);
  if (out == NULL) {
    return NULL;
  }
  out[0] = '\0';
  for (i = first; i < argc; i++) {
    if (i > first) {
      strcat(out, " ");
    }
    strcat(out, argv[i]);
  }
  return out;
}

int main(int argc, char const *const *argv) {
  amqp_connection_state_t conn = NULL;
  char *body = NULL;
  int exit_code = EXIT_FAILURE;

  body = argc > 2 ? join_args(2, argc, argv) : strdup("Hello World!");
  if (body == NULL) {
    fprintf(stderr, "Out of memory\n");
    return exit_code;
  }

  if (tutorial_connect(tutorial_broker_url(argc, argv), &conn) != TUTORIAL_OK) {
    goto out;
  }

  /* Durable queue: it survives a broker restart. The worker declares it with
   * the same arguments; redeclaring with different ones is a channel error. */
  amqp_queue_declare(conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(QUEUE_NAME), 0,
                     /*durable*/ 1, 0, 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Declaring queue") !=
      TUTORIAL_OK) {
    goto out;
  }

  {
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    /* A durable queue alone is not enough: the message itself must be marked
     * persistent to be written to disk. (For a hard guarantee that the broker
     * has taken responsibility for it, see the publisher confirms example.) */
    props.delivery_mode = AMQP_DELIVERY_PERSISTENT;

    if (tutorial_check_status(
            amqp_basic_publish(conn, TUTORIAL_CHANNEL, amqp_empty_bytes,
                               amqp_cstring_bytes(QUEUE_NAME),
                               /*mandatory*/ 1, /*immediate*/ 0, &props,
                               amqp_cstring_bytes(body)),
            "Publishing") != TUTORIAL_OK) {
      goto out;
    }
  }

  printf(" [x] Sent '%s'\n", body);
  exit_code = EXIT_SUCCESS;

out:
  tutorial_close(conn);
  free(body);
  return exit_code;
}
