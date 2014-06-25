/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
 *
 * Portions created by Alan Antonuk are Copyright (c) 2012-2013
 * Alan Antonuk. All Rights Reserved.
 *
 * Portions created by VMware are Copyright (c) 2007-2012 VMware, Inc.
 * All Rights Reserved.
 *
 * Portions created by Tony Garnock-Jones are Copyright (c) 2009-2010
 * VMware, Inc. and Tony Garnock-Jones. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ***** END LICENSE BLOCK *****
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "amqp_tcp_socket.h"
#include "amqp_private.h"
#include "amqp_timer.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef INITIAL_FRAME_POOL_PAGE_SIZE
#define INITIAL_FRAME_POOL_PAGE_SIZE 65536
#endif

#ifndef INITIAL_DECODING_POOL_PAGE_SIZE
#define INITIAL_DECODING_POOL_PAGE_SIZE 131072
#endif

#ifndef INITIAL_INBOUND_SOCK_BUFFER_SIZE
#define INITIAL_INBOUND_SOCK_BUFFER_SIZE 131072
#endif


#define ENFORCE_STATE(statevec, statenum)                                                 \
  {                                                                                       \
    amqp_connection_state_t _check_state = (statevec);                                    \
    size_t _wanted_state = (statenum);                                                    \
    if (_check_state->state != _wanted_state)                                             \
      amqp_abort("Programming error: invalid AMQP connection state: expected %d, got %d", \
                 _wanted_state,                                                           \
                 _check_state->state);                                                    \
  }

amqp_connection_state_t amqp_new_connection(void)
{
  int res;
  amqp_connection_state_t state =
    (amqp_connection_state_t) calloc(1, sizeof(struct amqp_connection_state_t_));

  if (state == NULL) {
    return NULL;
  }

  res = amqp_tune_connection(state, 0, INITIAL_FRAME_POOL_PAGE_SIZE, 0);
  if (0 != res) {
    goto out_nomem;
  }

  state->inbound_buffer.bytes = state->header_buffer;
  state->inbound_buffer.len = sizeof(state->header_buffer);

  state->state = CONNECTION_STATE_INITIAL;
  /* the server protocol version response is 8 bytes, which conveniently
     is also the minimum frame size */
  state->target_size = 8;

  state->sock_inbound_buffer.len = INITIAL_INBOUND_SOCK_BUFFER_SIZE;
  state->sock_inbound_buffer.bytes = malloc(INITIAL_INBOUND_SOCK_BUFFER_SIZE);
  if (state->sock_inbound_buffer.bytes == NULL) {
    goto out_nomem;
  }

  init_amqp_pool(&state->properties_pool, 512);

  return state;

out_nomem:
  free(state->sock_inbound_buffer.bytes);
  free(state);
  return NULL;
}

int amqp_get_sockfd(amqp_connection_state_t state)
{
  return state->socket ? amqp_socket_get_sockfd(state->socket) : -1;
}

void amqp_set_sockfd(amqp_connection_state_t state,
                     int sockfd)
{
  amqp_socket_t *socket = amqp_tcp_socket_new(state);
  if (!socket) {
    amqp_abort("%s", strerror(errno));
  }
  amqp_tcp_socket_set_sockfd(socket, sockfd);
}

void amqp_set_socket(amqp_connection_state_t state, amqp_socket_t *socket)
{
  amqp_socket_delete(state->socket);
  state->socket = socket;
}

amqp_socket_t *
amqp_get_socket(amqp_connection_state_t state)
{
  return state->socket;
}

int amqp_tune_connection(amqp_connection_state_t state,
                         int channel_max,
                         int frame_max,
                         int heartbeat)
{
  void *newbuf;

  ENFORCE_STATE(state, CONNECTION_STATE_IDLE);

  state->channel_max = channel_max;
  state->frame_max = frame_max;
  state->heartbeat = heartbeat;
  RABBIT_INFO("on connection: 0x%08X...", state);
  RABBIT_INFO("heartbeat is: %d", state->heartbeat);

  if (amqp_heartbeat_enabled(state)) {
    uint64_t current_time = amqp_get_monotonic_timestamp();
    if (0 == current_time) {
      return AMQP_STATUS_TIMER_FAILURE;
    }
    state->next_send_heartbeat = amqp_calc_next_send_heartbeat(state, current_time);
    state->next_recv_heartbeat = amqp_calc_next_recv_heartbeat(state, current_time);
  }

  state->outbound_buffer.len = frame_max;
  newbuf = realloc(state->outbound_buffer.bytes, frame_max);
  if (newbuf == NULL) {
    return AMQP_STATUS_NO_MEMORY;
  }
  state->outbound_buffer.bytes = newbuf;

  return AMQP_STATUS_OK;
}

int amqp_get_channel_max(amqp_connection_state_t state)
{
  return state->channel_max;
}

int amqp_destroy_connection(amqp_connection_state_t state)
{
  int status = AMQP_STATUS_OK;
  if (state) {
    int i;
    for (i = 0; i < POOL_TABLE_SIZE; ++i) {
      amqp_pool_table_entry_t *entry = state->pool_table[i];
      while (NULL != entry) {
        amqp_pool_table_entry_t *todelete = entry;
        empty_amqp_pool(&entry->pool);
        entry = entry->next;
        free(todelete);
      }
    }

    free(state->outbound_buffer.bytes);
    free(state->sock_inbound_buffer.bytes);
    amqp_socket_delete(state->socket);
    empty_amqp_pool(&state->properties_pool);
    free(state);
  }
  return status;
}

static void return_to_idle(amqp_connection_state_t state)
{
  state->inbound_buffer.len = sizeof(state->header_buffer);
  state->inbound_buffer.bytes = state->header_buffer;
  state->inbound_offset = 0;
  state->target_size = HEADER_SIZE;
  state->state = CONNECTION_STATE_IDLE;
}

static size_t consume_data(amqp_connection_state_t state,
                           amqp_bytes_t *received_data)
{
  /* how much data is available and will fit? */
  size_t bytes_consumed = state->target_size - state->inbound_offset;
  if (received_data->len < bytes_consumed) {
    bytes_consumed = received_data->len;
  }

  memcpy(amqp_offset(state->inbound_buffer.bytes, state->inbound_offset),
         received_data->bytes, bytes_consumed);
  state->inbound_offset += bytes_consumed;
  received_data->bytes = amqp_offset(received_data->bytes, bytes_consumed);
  received_data->len -= bytes_consumed;

  return bytes_consumed;
}

int amqp_handle_input(amqp_connection_state_t state,
                      amqp_bytes_t received_data,
                      amqp_frame_t *decoded_frame)
{
  size_t bytes_consumed;
  void *raw_frame;

  /* Returning frame_type of zero indicates either insufficient input,
     or a complete, ignored frame was read. */
  decoded_frame->frame_type = 0;

  if (received_data.len == 0) {
    RABBIT_INFO("return AMQP_STATUS_OK");
    return AMQP_STATUS_OK;
  }

  if (state->state == CONNECTION_STATE_IDLE) {
    state->state = CONNECTION_STATE_HEADER;
  }

  bytes_consumed = consume_data(state, &received_data);

#ifdef RABBIT_INFO_AVAILABLE
  char * raw_char = state->inbound_buffer.bytes;
  RABBIT_INFO( "state=%d inbound_offset=%d target_size=%d raw_char =%08x: %02x %02x %02x %02x %02x %02x %02x %02x", state->state, state->inbound_offset, state->target_size, (int)raw_char, (int)raw_char[0], (int)raw_char[1], (int)raw_char[2], (int)raw_char[3], (int)raw_char[4], (int)raw_char[5], (int)raw_char[6], (int)raw_char[7]);
#endif

  /* do we have target_size data yet? if not, return with the
     expectation that more will arrive */
  if (state->inbound_offset < state->target_size) {
    RABBIT_INFO("return %d", bytes_consumed);
    return bytes_consumed;
  }

  raw_frame = state->inbound_buffer.bytes;
  switch (state->state) {
  case CONNECTION_STATE_INITIAL:
    /* check for a protocol header from the server */
    if (memcmp(raw_frame, "AMQP", 4) == 0) {
      decoded_frame->frame_type = AMQP_PSEUDOFRAME_PROTOCOL_HEADER;
      decoded_frame->channel = 0;

      decoded_frame->payload.protocol_header.transport_high
        = amqp_d8(raw_frame, 4);
      decoded_frame->payload.protocol_header.transport_low
        = amqp_d8(raw_frame, 5);
      decoded_frame->payload.protocol_header.protocol_version_major
        = amqp_d8(raw_frame, 6);
      decoded_frame->payload.protocol_header.protocol_version_minor
        = amqp_d8(raw_frame, 7);

      return_to_idle(state);
      RABBIT_INFO("return %d", bytes_consumed);
      return bytes_consumed;
    }

    /* it's not a protocol header; fall through to process it as a
       regular frame header */

  case CONNECTION_STATE_HEADER:
    switch (decoded_frame->frame_type) {
      /* don't allow a corrupt frame type to allocate a huge block of memory.
       */
      default:
        return AMQP_STATUS_BAD_AMQP_DATA;
        break;

      case 0:
      case AMQP_FRAME_METHOD:
      case AMQP_FRAME_HEADER:
      case AMQP_FRAME_BODY:
      case AMQP_FRAME_HEARTBEAT: {
        amqp_channel_t channel;
        amqp_pool_t *channel_pool;
        /* frame length is 3 bytes in */
        channel = amqp_d16(raw_frame, 1);


        channel_pool = amqp_get_or_create_channel_pool(state, channel);
        if (NULL == channel_pool) {
          return AMQP_STATUS_NO_MEMORY;
        }

        /*
         * don't allow a corrupt frame size to allocate a huge block of memory.
         */
        size_t new_target_size = amqp_d32(raw_frame, 3) + HEADER_SIZE + FOOTER_SIZE;

        if (new_target_size > (size_t) state->frame_max) {
           return AMQP_STATUS_BAD_AMQP_DATA;
        }

        state->target_size = new_target_size;

        amqp_pool_alloc_bytes(channel_pool, state->target_size, &state->inbound_buffer);
        if (NULL == state->inbound_buffer.bytes) {
          return AMQP_STATUS_NO_MEMORY;
        }
        memcpy(state->inbound_buffer.bytes, state->header_buffer, HEADER_SIZE);
        raw_frame = state->inbound_buffer.bytes;

        state->state = CONNECTION_STATE_BODY;

        bytes_consumed += consume_data(state, &received_data);

#ifdef RABBIT_INFO_AVAILABLE
        char * raw_char2 = state->inbound_buffer.bytes;
        RABBIT_INFO( "state=%d inbound_offset=%d target_size=%d raw_char2=%08x: %02x %02x %02x %02x %02x %02x %02x %02x frame_type=%u channel=%d target_size=%d bytes_consumed=%d",
            state->state, state->inbound_offset, state->target_size, (int)raw_char2,
            (int)raw_char2[0], (int)raw_char2[1], (int)raw_char2[2], (int)raw_char2[3], (int)raw_char2[4], (int)raw_char2[5], (int)raw_char2[6], (int)raw_char2[7],
            (unsigned int)decoded_frame->frame_type, channel, new_target_size, bytes_consumed);
#endif

        /* do we have target_size data yet? if not, return with the
           expectation that more will arrive */
        if (state->inbound_offset < state->target_size) {
          RABBIT_INFO("return %d", bytes_consumed);
          return bytes_consumed;
        }
        /* fall through to process body */
      }
    }


  case CONNECTION_STATE_BODY: {
    amqp_bytes_t encoded;
    int res;
    amqp_pool_t *channel_pool;

    /* Check frame end marker (footer) */
    if (amqp_d8(raw_frame, state->target_size - 1) != AMQP_FRAME_END) {
      return AMQP_STATUS_BAD_AMQP_DATA;
    }

    decoded_frame->frame_type = amqp_d8(raw_frame, 0);
    decoded_frame->channel = amqp_d16(raw_frame, 1);

    channel_pool = amqp_get_or_create_channel_pool(state, decoded_frame->channel);
    if (NULL == channel_pool) {
      return AMQP_STATUS_NO_MEMORY;
    }

    switch (decoded_frame->frame_type) {
    case AMQP_FRAME_METHOD:
      decoded_frame->payload.method.id = amqp_d32(raw_frame, HEADER_SIZE);
      encoded.bytes = amqp_offset(raw_frame, HEADER_SIZE + 4);
      encoded.len = state->target_size - HEADER_SIZE - 4 - FOOTER_SIZE;

      res = amqp_decode_method(decoded_frame->payload.method.id,
                               channel_pool, encoded,
                               &decoded_frame->payload.method.decoded);
      if (res < 0) {
        RABBIT_INFO("return %d", res);
        return res;
      }

      break;

    case AMQP_FRAME_HEADER:
      decoded_frame->payload.properties.class_id
        = amqp_d16(raw_frame, HEADER_SIZE);
      /* unused 2-byte weight field goes here */
      decoded_frame->payload.properties.body_size
        = amqp_d64(raw_frame, HEADER_SIZE + 4);
      encoded.bytes = amqp_offset(raw_frame, HEADER_SIZE + 12);
      encoded.len = state->target_size - HEADER_SIZE - 12 - FOOTER_SIZE;
      decoded_frame->payload.properties.raw = encoded;

      res = amqp_decode_properties(decoded_frame->payload.properties.class_id,
                                   channel_pool, encoded,
                                   &decoded_frame->payload.properties.decoded);
      if (res < 0) {
        RABBIT_INFO("return %d", res);
        return res;
      }

      break;

    case AMQP_FRAME_BODY:
      decoded_frame->payload.body_fragment.len
        = state->target_size - HEADER_SIZE - FOOTER_SIZE;
      decoded_frame->payload.body_fragment.bytes
        = amqp_offset(raw_frame, HEADER_SIZE);
      break;

    case AMQP_FRAME_HEARTBEAT:
      break;

    default:
      /* Ignore the frame */
      decoded_frame->frame_type = 0;
      break;
    }

    return_to_idle(state);
    RABBIT_INFO("return %d", bytes_consumed);
    return bytes_consumed;
  }

  default:
    amqp_abort("Internal error: invalid amqp_connection_state_t->state %d", state->state);
    return bytes_consumed;
  }
}

amqp_boolean_t amqp_release_buffers_ok(amqp_connection_state_t state)
{
  return (state->state == CONNECTION_STATE_IDLE);
}

void amqp_release_buffers(amqp_connection_state_t state)
{
  int i;
  ENFORCE_STATE(state, CONNECTION_STATE_IDLE);

  for (i = 0; i < POOL_TABLE_SIZE; ++i) {
    amqp_pool_table_entry_t *entry = state->pool_table[i];

    for ( ;NULL != entry; entry = entry->next) {
      amqp_maybe_release_buffers_on_channel(state, entry->channel);
    }
  }
}

void amqp_maybe_release_buffers(amqp_connection_state_t state)
{
  if (amqp_release_buffers_ok(state)) {
    amqp_release_buffers(state);
  }
}

void amqp_maybe_release_buffers_on_channel(amqp_connection_state_t state, amqp_channel_t channel)
{
  amqp_link_t *queued_link;
  amqp_pool_t *pool;
  if (CONNECTION_STATE_IDLE != state->state) {
    return;
  }

  queued_link = state->first_queued_frame;

  while (NULL != queued_link) {
    amqp_frame_t *frame = queued_link->data;
    if (channel == frame->channel) {
      return;
    }

    queued_link = queued_link->next;
  }

  pool = amqp_get_channel_pool(state, channel);

  if (pool != NULL) {
    recycle_amqp_pool(pool);
  }
}

int amqp_send_frame(amqp_connection_state_t state,
                    const amqp_frame_t *frame)
{
  void *out_frame = state->outbound_buffer.bytes;
  int res;

  amqp_e8(out_frame, 0, frame->frame_type);
  amqp_e16(out_frame, 1, frame->channel);

  if (frame->frame_type == AMQP_FRAME_BODY) {
    /* For a body frame, rather than copying data around, we use
       writev to compose the frame */
    struct iovec iov[3];
    uint8_t frame_end_byte = AMQP_FRAME_END;
    const amqp_bytes_t *body = &frame->payload.body_fragment;

    amqp_e32(out_frame, 3, body->len);

    iov[0].iov_base = out_frame;
    iov[0].iov_len = HEADER_SIZE;
    iov[1].iov_base = body->bytes;
    iov[1].iov_len = body->len;
    iov[2].iov_base = (char *)&frame_end_byte;
    iov[2].iov_len = FOOTER_SIZE;

    RABBIT_INFO("writev body->len=%d", body->len);
    res = amqp_socket_writev(state->socket, iov, 3);
    RABBIT_INFO("writev body->len=%d res=%d", body->len, res);
  } else {
    size_t out_frame_len;
    amqp_bytes_t encoded;

    switch (frame->frame_type) {
    case AMQP_FRAME_METHOD:
      amqp_e32(out_frame, HEADER_SIZE, frame->payload.method.id);

      encoded.bytes = amqp_offset(out_frame, HEADER_SIZE + 4);
      encoded.len = state->outbound_buffer.len - HEADER_SIZE - 4 - FOOTER_SIZE;

      RABBIT_INFO("amqp_encode_method out_frame=%08x len=%d method_id=%d decoded=%08x encoded=%08x",
          (int)out_frame, (int)encoded.len, (int)frame->payload.method.id, (int)frame->payload.method.decoded, (int)&encoded);
      res = amqp_encode_method(frame->payload.method.id,
                               frame->payload.method.decoded, encoded);
      RABBIT_INFO("amqp_encode_method out_frame=%08x len=%d method_id=%d decoded=%08x encoded=%08x res=%d",
          (int)out_frame, (int)encoded.len, (int)frame->payload.method.id, (int)frame->payload.method.decoded, (int)&encoded, res);

      if (res < 0) {
        return res;
      }

      out_frame_len = res + 4;
      break;

    case AMQP_FRAME_HEADER:
      amqp_e16(out_frame, HEADER_SIZE, frame->payload.properties.class_id);
      amqp_e16(out_frame, HEADER_SIZE+2, 0); /* "weight" */
      amqp_e64(out_frame, HEADER_SIZE+4, frame->payload.properties.body_size);

      encoded.bytes = amqp_offset(out_frame, HEADER_SIZE + 12);
      encoded.len = state->outbound_buffer.len - HEADER_SIZE - 12 - FOOTER_SIZE;

      RABBIT_INFO("amqp_encode_properties out_frame=%08x len=%d class_id=%d decoded=%08x encoded=%08x",
          (int)out_frame, encoded.len, (int)(frame->payload.properties.class_id), (int)(frame->payload.properties.decoded), (int)&encoded);

      res = amqp_encode_properties(frame->payload.properties.class_id,
                                   frame->payload.properties.decoded, encoded);
      RABBIT_INFO("amqp_encode_properties out_frame=%08x len=%d class_id=%d decoded=%08x encoded=%08x res=%d",
          (int)out_frame, encoded.len, (int)(frame->payload.properties.class_id), (int)(frame->payload.properties.decoded), (int)&encoded, res);

      if (res < 0) {
        return res;
      }

      out_frame_len = res + 12;
      break;

    case AMQP_FRAME_HEARTBEAT:
      RABBIT_INFO("send heartbeat");
      out_frame_len = 0;
      break;

    default:
      RABBIT_INFO("");
      return AMQP_STATUS_INVALID_PARAMETER;
    }

    amqp_e32(out_frame, 3, out_frame_len);
    amqp_e8(out_frame, out_frame_len + HEADER_SIZE, AMQP_FRAME_END);
    RABBIT_INFO("send socket=%08x, outframe=%08x, len=%d", state->socket, out_frame, out_frame_len + HEADER_SIZE + FOOTER_SIZE);
    res = amqp_socket_send(state->socket, out_frame,
                           out_frame_len + HEADER_SIZE + FOOTER_SIZE);
    RABBIT_INFO("send socket=%08x, outframe=%08x, len=%d res=%d", state->socket, out_frame, out_frame_len + HEADER_SIZE + FOOTER_SIZE, res);
  }

  if (state->heartbeat > 0) {
    uint64_t current_time = amqp_get_monotonic_timestamp();
    if (0 == current_time) {
      return AMQP_STATUS_TIMER_FAILURE;
    }
    state->next_send_heartbeat = amqp_calc_next_send_heartbeat(state, current_time);
  }

  return res;
}
