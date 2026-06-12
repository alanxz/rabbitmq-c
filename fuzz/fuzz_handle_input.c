// Copyright 2007 - 2026, Arthur Chan and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>

// Drives amqp_handle_input(), the byte->frame wire parser, feeding the input one
// frame at a time and advancing by the number of bytes it reports consuming.
extern int LLVMFuzzerTestOneInput(const char *data, size_t size) {

  amqp_connection_state_t conn;
  amqp_bytes_t buffer;
  size_t iterations = 0;

  if (size == 0) {
    return 0;
  }

  conn = amqp_new_connection();
  if (conn == NULL) {
    return 0;
  }

  buffer.bytes = (void *)data;
  buffer.len = size;

  while (buffer.len > 0 && iterations < 4096) {
    amqp_frame_t frame;
    int res;

    memset(&frame, 0, sizeof(frame));
    res = amqp_handle_input(conn, buffer, &frame);
    if (res <= 0) {
      break;
    }

    buffer.bytes = (void *)((const char *)buffer.bytes + (size_t)res);
    buffer.len -= (size_t)res;

    if (frame.frame_type != 0) {
      amqp_maybe_release_buffers_on_channel(conn, frame.channel);
    }

    iterations++;
  }

  amqp_destroy_connection(conn);
  return 0;
}
