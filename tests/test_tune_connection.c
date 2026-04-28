// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include "amqp_private.h"
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>

#include <stdio.h>
#include <stdlib.h>

static void expect_frame_max(int requested, int expected) {
  int res;
  amqp_connection_state_t state = amqp_new_connection();

  if (state == NULL) {
    fprintf(stderr, "amqp_new_connection failed\n");
    abort();
  }

  state->state = CONNECTION_STATE_IDLE;

  res = amqp_tune_connection(state, 0, requested, 0);
  if (res != AMQP_STATUS_OK) {
    fprintf(stderr, "amqp_tune_connection returned %d\n", res);
    abort();
  }

  if (amqp_get_frame_max(state) != expected) {
    fprintf(stderr, "expected frame_max %d, got %d\n", expected,
            amqp_get_frame_max(state));
    abort();
  }

  if (state->outbound_buffer.len != (size_t)expected) {
    fprintf(stderr, "expected outbound buffer length %d, got %zu\n", expected,
            state->outbound_buffer.len);
    abort();
  }

  amqp_destroy_connection(state);
}

int main(void) {
  expect_frame_max(0, AMQP_FRAME_MIN_SIZE);
  expect_frame_max(1, AMQP_FRAME_MIN_SIZE);
  expect_frame_max(AMQP_FRAME_MIN_SIZE - 1, AMQP_FRAME_MIN_SIZE);
  expect_frame_max(AMQP_FRAME_MIN_SIZE, AMQP_FRAME_MIN_SIZE);
  expect_frame_max(AMQP_DEFAULT_FRAME_SIZE, AMQP_DEFAULT_FRAME_SIZE);

  return 0;
}
