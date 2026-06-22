// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include "amqp_private.h"
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Regression test for GHSA-hfjv-vcp3-39wh: passing an oversized
 * AMQP_FRAME_BODY to amqp_send_frame() must not overflow the outbound
 * buffer. It should be rejected with AMQP_STATUS_BAD_AMQP_DATA. */
static void test_oversized_body_frame_rejected(void) {
  amqp_connection_state_t state = amqp_new_connection();
  amqp_frame_t frame;
  size_t body_len;
  char *body;
  int res;

  if (state == NULL) {
    fprintf(stderr, "amqp_new_connection failed\n");
    abort();
  }

  /* The default outbound buffer is AMQP_DEFAULT_FRAME_SIZE bytes; use a
   * body fragment that is larger than that buffer can hold. */
  body_len = state->outbound_buffer.len + 1024;
  body = malloc(body_len);
  if (body == NULL) {
    fprintf(stderr, "malloc failed\n");
    abort();
  }
  memset(body, 'A', body_len);

  memset(&frame, 0, sizeof(frame));
  frame.frame_type = AMQP_FRAME_BODY;
  frame.channel = 1;
  frame.payload.body_fragment.bytes = body;
  frame.payload.body_fragment.len = body_len;

  res = amqp_send_frame(state, &frame);
  if (res != AMQP_STATUS_BAD_AMQP_DATA) {
    fprintf(stderr, "expected AMQP_STATUS_BAD_AMQP_DATA (%d), got %d\n",
            AMQP_STATUS_BAD_AMQP_DATA, res);
    abort();
  }

  free(body);
  amqp_destroy_connection(state);
}

int main(void) {
  test_oversized_body_frame_rejected();
  return 0;
}
