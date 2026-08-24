// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/* Regression test for a client-side memory-exhaustion DoS: a peer sending
 * frames on channels beyond the negotiated channel_max, or a sustained
 * stream of frames with an unrecognized frame type, could make
 * amqp_handle_input() retain a per-channel memory pool page for every such
 * frame instead of rejecting or recycling it. */

#include "amqp_private.h"
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static amqp_connection_state_t new_ready_state(int channel_max,
                                               int frame_max) {
  amqp_connection_state_t state = amqp_new_connection();
  if (state == NULL) {
    fprintf(stderr, "amqp_new_connection failed\n");
    abort();
  }

  /* Put the connection past the initial protocol-header stage so
   * amqp_handle_input() parses ordinary frames, mirroring what the
   * (private) return_to_idle() helper sets up. */
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

/* A frame on a channel greater than the negotiated channel_max must be
 * rejected before a channel pool is created for it. */
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

/* channel_max == 0 means "no limit"; frames on any channel should still be
 * accepted (and ignored, since the frame type is unrecognized). */
static void test_channel_max_zero_means_unlimited(void) {
  amqp_connection_state_t state = new_ready_state(0, AMQP_FRAME_MIN_SIZE);
  amqp_frame_t frame;
  amqp_bytes_t data;
  uint8_t buf[8];
  int res;

  data.len = build_empty_frame(buf, 0xff, 12345);
  data.bytes = buf;

  res = amqp_handle_input(state, data, &frame);
  if (res != (int)data.len) {
    fprintf(stderr, "expected frame to be consumed (%d), got %d\n",
           (int)data.len, res);
    abort();
  }
  if (frame.frame_type != 0) {
    fprintf(stderr, "expected unrecognized frame type to be ignored\n");
    abort();
  }

  amqp_destroy_connection(state);
}

/* A heartbeat frame must be on channel 0. */
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

/* A sustained stream of small frames with an unrecognized frame type on a
 * single, permitted channel must not accumulate pool pages: each ignored
 * frame's allocation should be recycled, so the channel pool never grows
 * past its first page. */
static void test_ignored_frames_do_not_leak_pool_pages(void) {
  amqp_connection_state_t state = new_ready_state(1, AMQP_FRAME_MIN_SIZE);
  amqp_pool_t *pool;
  int i;

  for (i = 0; i < 5000; ++i) {
    amqp_frame_t frame;
    amqp_bytes_t data;
    uint8_t buf[8];
    int res;

    data.len = build_empty_frame(buf, 0xff, 1);
    data.bytes = buf;

    res = amqp_handle_input(state, data, &frame);
    if (res != (int)data.len) {
      fprintf(stderr, "frame %d: expected %d consumed, got %d\n", i,
             (int)data.len, res);
      abort();
    }
    if (frame.frame_type != 0) {
      fprintf(stderr, "frame %d: expected frame to be ignored\n", i);
      abort();
    }
  }

  pool = amqp_get_channel_pool(state, 1);
  if (pool == NULL) {
    fprintf(stderr, "expected a channel pool to have been created\n");
    abort();
  }
  if (pool->pages.num_blocks > 1) {
    fprintf(stderr,
           "channel pool grew to %d pages after 5000 ignored frames; "
           "pool pages are not being recycled\n",
           pool->pages.num_blocks);
    abort();
  }

  amqp_destroy_connection(state);
}

int main(void) {
  test_channel_exceeding_channel_max_rejected();
  test_channel_max_zero_means_unlimited();
  test_heartbeat_on_nonzero_channel_rejected();
  test_ignored_frames_do_not_leak_pool_pages();
  return 0;
}
