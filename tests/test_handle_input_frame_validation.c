// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/* Regression test for a client-side memory-exhaustion DoS: frames on
 * channels beyond channel_max, or with an unrecognized frame type, were
 * accepted instead of rejected, letting a peer grow per-channel pool
 * memory without bound. */

#include "amqp_private.h"
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>

#include <stdio.h>
#include <stdlib.h>

static amqp_connection_state_t new_ready_state(int channel_max, int frame_max) {
  amqp_connection_state_t state = amqp_new_connection();
  if (state == NULL) {
    fprintf(stderr, "amqp_new_connection failed\n");
    abort();
  }

  /* Mirrors the private return_to_idle() helper, so amqp_handle_input()
   * parses an ordinary frame instead of expecting a protocol header. */
  state->inbound_buffer.bytes = state->header_buffer;
  state->inbound_buffer.len = sizeof(state->header_buffer);
  state->inbound_offset = 0;
  state->target_size = HEADER_SIZE;
  state->state = CONNECTION_STATE_IDLE;
  state->channel_max = channel_max;
  state->frame_max = frame_max;

  return state;
}

/* Builds a minimal frame: header (type, channel, size) + empty payload +
 * frame-end footer. */
static size_t build_empty_frame(uint8_t *buf, uint8_t frame_type,
                                uint16_t channel) {
  buf[0] = frame_type;
  amqp_e16(channel, buf + 1);
  amqp_e32(0, buf + 3);
  buf[7] = AMQP_FRAME_END;
  return 8;
}

static void test_channel_exceeding_channel_max_rejected(void) {
  amqp_connection_state_t state = new_ready_state(1, AMQP_FRAME_MIN_SIZE);
  amqp_frame_t frame;
  amqp_bytes_t data;
  uint8_t buf[8];
  int res;

  data.len = build_empty_frame(buf, 0xff, 2);
  data.bytes = buf;

  res = amqp_handle_input(state, data, &frame);
  if (res != AMQP_STATUS_BAD_AMQP_DATA) {
    fprintf(stderr,
            "expected AMQP_STATUS_BAD_AMQP_DATA (%d) for channel > "
            "channel_max, got %d\n",
            AMQP_STATUS_BAD_AMQP_DATA, res);
    abort();
  }

  if (amqp_get_channel_pool(state, 2) != NULL) {
    fprintf(stderr,
            "channel pool was created for a channel beyond channel_max\n");
    abort();
  }

  amqp_destroy_connection(state);
}

/* channel_max == 0 means no explicit limit, which per spec caps channels
 * at 65535 rather than leaving them unbounded. */
static void test_channel_max_zero_means_protocol_max(void) {
  amqp_connection_state_t state = new_ready_state(0, AMQP_FRAME_MIN_SIZE);
  amqp_frame_t frame;
  amqp_bytes_t data;
  uint8_t buf[8];
  int res;

  data.len = build_empty_frame(buf, AMQP_FRAME_BODY, 65535);
  data.bytes = buf;

  res = amqp_handle_input(state, data, &frame);
  if (res != (int)data.len) {
    fprintf(stderr, "expected frame to be consumed (%d), got %d\n",
            (int)data.len, res);
    abort();
  }
  if (frame.frame_type != AMQP_FRAME_BODY || frame.channel != 65535) {
    fprintf(stderr, "expected a body frame on channel 65535 to be accepted\n");
    abort();
  }

  amqp_destroy_connection(state);
}

static void test_heartbeat_on_nonzero_channel_rejected(void) {
  amqp_connection_state_t state = new_ready_state(0, AMQP_FRAME_MIN_SIZE);
  amqp_frame_t frame;
  amqp_bytes_t data;
  uint8_t buf[8];
  int res;

  data.len = build_empty_frame(buf, AMQP_FRAME_HEARTBEAT, 1);
  data.bytes = buf;

  res = amqp_handle_input(state, data, &frame);
  if (res != AMQP_STATUS_BAD_AMQP_DATA) {
    fprintf(stderr,
            "expected AMQP_STATUS_BAD_AMQP_DATA (%d) for heartbeat on "
            "non-zero channel, got %d\n",
            AMQP_STATUS_BAD_AMQP_DATA, res);
    abort();
  }

  amqp_destroy_connection(state);
}

int main(void) {
  test_channel_exceeding_channel_max_rejected();
  test_channel_max_zero_means_protocol_max();
  test_heartbeat_on_nonzero_channel_rejected();
  return 0;
}
