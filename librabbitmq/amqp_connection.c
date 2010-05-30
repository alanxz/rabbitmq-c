/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * The Original Code is librabbitmq.
 *
 * The Initial Developers of the Original Code are LShift Ltd, Cohesive
 * Financial Technologies LLC, and Rabbit Technologies Ltd.  Portions
 * created before 22-Nov-2008 00:00:00 GMT by LShift Ltd, Cohesive
 * Financial Technologies LLC, or Rabbit Technologies Ltd are Copyright
 * (C) 2007-2008 LShift Ltd, Cohesive Financial Technologies LLC, and
 * Rabbit Technologies Ltd.
 *
 * Portions created by LShift Ltd are Copyright (C) 2007-2009 LShift
 * Ltd. Portions created by Cohesive Financial Technologies LLC are
 * Copyright (C) 2007-2009 Cohesive Financial Technologies
 * LLC. Portions created by Rabbit Technologies Ltd are Copyright (C)
 * 2007-2009 Rabbit Technologies Ltd.
 *
 * Portions created by Tony Garnock-Jones are Copyright (C) 2009-2010
 * LShift Ltd and Tony Garnock-Jones.
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2 or later (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GPL, and not to allow others to use your
 * version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the
 * notice and other provisions required by the GPL. If you do not
 * delete the provisions above, a recipient may use your version of
 * this file under the terms of any one of the MPL or the GPL.
 *
 * ***** END LICENSE BLOCK *****
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>

#include "amqp.h"
#include "amqp_framing.h"
#include "amqp_private.h"

#include <assert.h>

#define INITIAL_FRAME_POOL_PAGE_SIZE 65536
#define INITIAL_DECODING_POOL_PAGE_SIZE 131072
#define INITIAL_INBOUND_SOCK_BUFFER_SIZE 131072

#define ENFORCE_STATE(statevec, statenum)				\
  {									\
    amqp_connection_state_t _check_state = (statevec);			\
    int _wanted_state = (statenum);					\
    amqp_assert(_check_state->state == _wanted_state,			\
		"Programming error: invalid AMQP connection state: expected %d, got %d", \
		_wanted_state,						\
		_check_state->state);					\
  }

amqp_connection_state_t amqp_new_connection(void) {
  amqp_connection_state_t state =
    (amqp_connection_state_t) calloc(1, sizeof(struct amqp_connection_state_t_));

  if (state == NULL) {
    return NULL;
  }

  init_amqp_pool(&state->frame_pool, INITIAL_FRAME_POOL_PAGE_SIZE);
  init_amqp_pool(&state->decoding_pool, INITIAL_DECODING_POOL_PAGE_SIZE);

  state->state = CONNECTION_STATE_IDLE;

  state->inbound_buffer.bytes = NULL;
  state->outbound_buffer.bytes = NULL;
  if (amqp_tune_connection(state, 0, INITIAL_FRAME_POOL_PAGE_SIZE, 0) != 0) {
    empty_amqp_pool(&state->frame_pool);
    empty_amqp_pool(&state->decoding_pool);
    free(state);
    return NULL;
  }

  state->inbound_offset = 0;
  state->target_size = HEADER_SIZE;

  state->sockfd = -1;
  state->sock_inbound_buffer.len = INITIAL_INBOUND_SOCK_BUFFER_SIZE;
  state->sock_inbound_buffer.bytes = malloc(INITIAL_INBOUND_SOCK_BUFFER_SIZE);
  if (state->sock_inbound_buffer.bytes == NULL) {
    amqp_destroy_connection(state);
    return NULL;
  }

  state->sock_inbound_offset = 0;
  state->sock_inbound_limit = 0;

  state->first_queued_frame = NULL;
  state->last_queued_frame = NULL;

  return state;
}

int amqp_get_sockfd(amqp_connection_state_t state) {
  return state->sockfd;
}

void amqp_set_sockfd(amqp_connection_state_t state,
		     int sockfd)
{
  state->sockfd = sockfd;
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

  empty_amqp_pool(&state->frame_pool);
  init_amqp_pool(&state->frame_pool, frame_max);

  state->inbound_buffer.len = frame_max;
  state->outbound_buffer.len = frame_max;
  newbuf = realloc(state->outbound_buffer.bytes, frame_max);
  if (newbuf == NULL) {
    amqp_destroy_connection(state);
    return -ENOMEM;
  }
  state->outbound_buffer.bytes = newbuf;

  return 0;
}

int amqp_get_channel_max(amqp_connection_state_t state) {
  return state->channel_max;
}

void amqp_destroy_connection(amqp_connection_state_t state) {
  empty_amqp_pool(&state->frame_pool);
  empty_amqp_pool(&state->decoding_pool);
  free(state->outbound_buffer.bytes);
  free(state->sock_inbound_buffer.bytes);
  free(state);
}

int amqp_end_connection(amqp_connection_state_t state) {
  int s = state->sockfd;
  amqp_destroy_connection(state);
  if (close(s) < 0)
    return -errno;
  else
    return 0;
} 

static void return_to_idle(amqp_connection_state_t state) {
  state->inbound_buffer.bytes = NULL;
  state->inbound_offset = 0;
  state->target_size = HEADER_SIZE;
  state->state = CONNECTION_STATE_IDLE;
}

int amqp_handle_input(amqp_connection_state_t state,
		      amqp_bytes_t received_data,
		      amqp_frame_t *decoded_frame)
{
  int total_bytes_consumed = 0;
  int bytes_consumed;

  /* Returning frame_type of zero indicates either insufficient input,
     or a complete, ignored frame was read. */
  decoded_frame->frame_type = 0;

 read_more:
  if (received_data.len == 0) {
    return total_bytes_consumed;
  }

  if (state->state == CONNECTION_STATE_IDLE) {
    state->inbound_buffer.bytes = amqp_pool_alloc(&state->frame_pool, state->inbound_buffer.len);
    if (state->inbound_buffer.bytes == NULL) {
      /* state->inbound_buffer.len is always nonzero, because it
	 corresponds to frame_max, which is not permitted to be less
	 than AMQP_FRAME_MIN_SIZE (currently 4096 bytes). */
      return -ENOMEM;
    }
    state->state = CONNECTION_STATE_WAITING_FOR_HEADER;
  }

  bytes_consumed = state->target_size - state->inbound_offset;
  if (received_data.len < bytes_consumed) {
    bytes_consumed = received_data.len;
  }

  E_BYTES(state->inbound_buffer, state->inbound_offset, bytes_consumed, received_data.bytes);
  state->inbound_offset += bytes_consumed;
  total_bytes_consumed += bytes_consumed;

  assert(state->inbound_offset <= state->target_size);

  if (state->inbound_offset < state->target_size) {
    return total_bytes_consumed;
  }

  switch (state->state) {
    case CONNECTION_STATE_WAITING_FOR_HEADER:
      if (D_8(state->inbound_buffer, 0) == AMQP_PSEUDOFRAME_PROTOCOL_HEADER &&
	  D_16(state->inbound_buffer, 1) == AMQP_PSEUDOFRAME_PROTOCOL_CHANNEL)
      {
	state->target_size = 8;
	state->state = CONNECTION_STATE_WAITING_FOR_PROTOCOL_HEADER;
      } else {
	state->target_size = D_32(state->inbound_buffer, 3) + HEADER_SIZE + FOOTER_SIZE;
	state->state = CONNECTION_STATE_WAITING_FOR_BODY;
      }

      /* Wind buffer forward, and try to read some body out of it. */
      received_data.len -= bytes_consumed;
      received_data.bytes = ((char *) received_data.bytes) + bytes_consumed;
      goto read_more;

    case CONNECTION_STATE_WAITING_FOR_BODY: {
      int frame_type = D_8(state->inbound_buffer, 0);

#if 0
      printf("recving:\n");
      amqp_dump(state->inbound_buffer.bytes, state->target_size);
#endif

      /* Check frame end marker (footer) */
      if (D_8(state->inbound_buffer, state->target_size - 1) != AMQP_FRAME_END) {
	return -EINVAL;
      }

      decoded_frame->channel = D_16(state->inbound_buffer, 1);

      switch (frame_type) {
	case AMQP_FRAME_METHOD: {
	  amqp_bytes_t encoded;

	  /* Four bytes of method ID before the method args. */
	  encoded.len = state->target_size - (HEADER_SIZE + 4 + FOOTER_SIZE);
	  encoded.bytes = D_BYTES(state->inbound_buffer, HEADER_SIZE + 4, encoded.len);

	  decoded_frame->frame_type = AMQP_FRAME_METHOD;
	  decoded_frame->payload.method.id = D_32(state->inbound_buffer, HEADER_SIZE);
	  AMQP_CHECK_RESULT(amqp_decode_method(decoded_frame->payload.method.id,
					       &state->decoding_pool,
					       encoded,
					       &decoded_frame->payload.method.decoded));
	  break;
	}

	case AMQP_FRAME_HEADER: {
	  amqp_bytes_t encoded;

	  /* 12 bytes for properties header. */
	  encoded.len = state->target_size - (HEADER_SIZE + 12 + FOOTER_SIZE);
	  encoded.bytes = D_BYTES(state->inbound_buffer, HEADER_SIZE + 12, encoded.len);

	  decoded_frame->frame_type = AMQP_FRAME_HEADER;
	  decoded_frame->payload.properties.class_id = D_16(state->inbound_buffer, HEADER_SIZE);
	  decoded_frame->payload.properties.body_size = D_64(state->inbound_buffer, HEADER_SIZE+4);
	  decoded_frame->payload.properties.raw = encoded;
	  AMQP_CHECK_RESULT(amqp_decode_properties(decoded_frame->payload.properties.class_id,
						   &state->decoding_pool,
						   encoded,
						   &decoded_frame->payload.properties.decoded));
	  break;
	}

	case AMQP_FRAME_BODY: {
	  size_t fragment_len = state->target_size - (HEADER_SIZE + FOOTER_SIZE);

	  decoded_frame->frame_type = AMQP_FRAME_BODY;
	  decoded_frame->payload.body_fragment.len = fragment_len;
	  decoded_frame->payload.body_fragment.bytes =
	    D_BYTES(state->inbound_buffer, HEADER_SIZE, fragment_len);
	  break;
	}

	case AMQP_FRAME_HEARTBEAT:
	  decoded_frame->frame_type = AMQP_FRAME_HEARTBEAT;
	  break;

	default:
	  /* Ignore the frame by not changing frame_type away from 0. */
	  break;
      }

      return_to_idle(state);
      return total_bytes_consumed;
    }

    case CONNECTION_STATE_WAITING_FOR_PROTOCOL_HEADER:
      decoded_frame->frame_type = AMQP_PSEUDOFRAME_PROTOCOL_HEADER;
      decoded_frame->channel = AMQP_PSEUDOFRAME_PROTOCOL_CHANNEL;
      amqp_assert(D_8(state->inbound_buffer, 3) == (uint8_t) 'P',
		  "Invalid protocol header received");
      decoded_frame->payload.protocol_header.transport_high = D_8(state->inbound_buffer, 4);
      decoded_frame->payload.protocol_header.transport_low = D_8(state->inbound_buffer, 5);
      decoded_frame->payload.protocol_header.protocol_version_major = D_8(state->inbound_buffer, 6);
      decoded_frame->payload.protocol_header.protocol_version_minor = D_8(state->inbound_buffer, 7);

      return_to_idle(state);
      return total_bytes_consumed;

    default:
      amqp_assert(0, "Internal error: invalid amqp_connection_state_t->state %d", state->state);
  }
}

amqp_boolean_t amqp_release_buffers_ok(amqp_connection_state_t state) {
  return (state->state == CONNECTION_STATE_IDLE) && (state->first_queued_frame == NULL);
}

void amqp_release_buffers(amqp_connection_state_t state) {
  ENFORCE_STATE(state, CONNECTION_STATE_IDLE);

  amqp_assert(state->first_queued_frame == NULL,
	      "Programming error: attempt to amqp_release_buffers while waiting events enqueued");

  recycle_amqp_pool(&state->frame_pool);
  recycle_amqp_pool(&state->decoding_pool);
}

void amqp_maybe_release_buffers(amqp_connection_state_t state) {
  if (amqp_release_buffers_ok(state)) {
    amqp_release_buffers(state);
  }
}

static int inner_send_frame(amqp_connection_state_t state,
			    amqp_frame_t const *frame,
			    amqp_bytes_t *encoded,
			    int *payload_len)
{
  int separate_body;

  E_8(state->outbound_buffer, 0, frame->frame_type);
  E_16(state->outbound_buffer, 1, frame->channel);
  switch (frame->frame_type) {
    case AMQP_FRAME_METHOD:
      E_32(state->outbound_buffer, HEADER_SIZE, frame->payload.method.id);
      encoded->len = state->outbound_buffer.len - (HEADER_SIZE + 4 + FOOTER_SIZE);
      encoded->bytes = D_BYTES(state->outbound_buffer, HEADER_SIZE + 4, encoded->len);
      *payload_len = AMQP_CHECK_RESULT(amqp_encode_method(frame->payload.method.id,
							  frame->payload.method.decoded,
							  *encoded)) + 4;
      separate_body = 0;
      break;

    case AMQP_FRAME_HEADER:
      E_16(state->outbound_buffer, HEADER_SIZE, frame->payload.properties.class_id);
      E_16(state->outbound_buffer, HEADER_SIZE+2, 0); /* "weight" */
      E_64(state->outbound_buffer, HEADER_SIZE+4, frame->payload.properties.body_size);
      encoded->len = state->outbound_buffer.len - (HEADER_SIZE + 12 + FOOTER_SIZE);
      encoded->bytes = D_BYTES(state->outbound_buffer, HEADER_SIZE + 12, encoded->len);
      *payload_len = AMQP_CHECK_RESULT(amqp_encode_properties(frame->payload.properties.class_id,
							      frame->payload.properties.decoded,
							      *encoded)) + 12;
      separate_body = 0;
      break;

    case AMQP_FRAME_BODY:
      *encoded = frame->payload.body_fragment;
      *payload_len = encoded->len;
      separate_body = 1;
      break;

    case AMQP_FRAME_HEARTBEAT:
      *encoded = AMQP_EMPTY_BYTES;
      *payload_len = 0;
      separate_body = 0;
      break;

    default:
      return -EINVAL;
  }

  E_32(state->outbound_buffer, 3, *payload_len);
  if (!separate_body) {
    E_8(state->outbound_buffer, *payload_len + HEADER_SIZE, AMQP_FRAME_END);
  }

#if 0
  if (separate_body) {
    printf("sending body frame (header):\n");
    amqp_dump(state->outbound_buffer.bytes, HEADER_SIZE);
    printf("sending body frame (payload):\n");
    amqp_dump(encoded->bytes, *payload_len);
  } else {
    printf("sending:\n");
    amqp_dump(state->outbound_buffer.bytes, *payload_len + HEADER_SIZE + FOOTER_SIZE);
  }
#endif

  return separate_body;
}

int amqp_send_frame(amqp_connection_state_t state,
		    amqp_frame_t const *frame)
{
  amqp_bytes_t encoded;
  int payload_len;
  int separate_body;

  separate_body = inner_send_frame(state, frame, &encoded, &payload_len);
  switch (separate_body) {
    case 0:
      AMQP_CHECK_RESULT(write(state->sockfd,
			      state->outbound_buffer.bytes,
			      payload_len + (HEADER_SIZE + FOOTER_SIZE)));
      return 0;

    case 1: {
      struct iovec iov[3];
      char frame_end_byte = AMQP_FRAME_END;
      iov[0].iov_base = state->outbound_buffer.bytes;
      iov[0].iov_len = HEADER_SIZE;
      iov[1].iov_base = encoded.bytes;
      iov[1].iov_len = payload_len;
      iov[2].iov_base = &frame_end_byte;
      assert(FOOTER_SIZE == 1);
      iov[2].iov_len = FOOTER_SIZE;
      AMQP_CHECK_RESULT(writev(state->sockfd, &iov[0], 3));
      return 0;
    }

    default:
      return separate_body;
  }
}

int amqp_send_frame_to(amqp_connection_state_t state,
		       amqp_frame_t const *frame,
		       amqp_output_fn_t fn,
		       void *context)
{
  amqp_bytes_t encoded;
  int payload_len;
  int separate_body;

  separate_body = inner_send_frame(state, frame, &encoded, &payload_len);
  switch (separate_body) {
    case 0:
      AMQP_CHECK_RESULT(fn(context,
			   state->outbound_buffer.bytes,
			   payload_len + (HEADER_SIZE + FOOTER_SIZE)));
      return 0;

    case 1:
      AMQP_CHECK_RESULT(fn(context, state->outbound_buffer.bytes, HEADER_SIZE));
      AMQP_CHECK_RESULT(fn(context, encoded.bytes, payload_len));
      {
	assert(FOOTER_SIZE == 1);
	char frame_end_byte = AMQP_FRAME_END;
	AMQP_CHECK_RESULT(fn(context, &frame_end_byte, FOOTER_SIZE));
      }
      return 0;

    default:
      return separate_body;
  }
}
