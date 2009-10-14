#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>

#include "amqp.h"
#include "amqp_framing.h"
#include "amqp_private.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include <assert.h>

int amqp_open_socket(char const *hostname,
		     int portnumber)
{
  int sockfd;
  struct sockaddr_in addr;
  struct hostent *he;

  he = gethostbyname(hostname);
  if (he == NULL) {
    return -ENOENT;
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(portnumber);
  addr.sin_addr.s_addr = * (uint32_t *) he->h_addr_list[0];

  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    int result = -errno;
    close(sockfd);
    return result;
  }

  return sockfd;
}

static char *header() {
  static char header[8];
  header[0] = 'A';
  header[1] = 'M';
  header[2] = 'Q';
  header[3] = 'P';
  header[4] = 1;
  header[5] = 1;
  header[6] = AMQP_PROTOCOL_VERSION_MAJOR;
  header[7] = AMQP_PROTOCOL_VERSION_MINOR;
  return header;
}

int amqp_send_header(amqp_connection_state_t state) {
  return write(state->sockfd, header(), 8);
}

int amqp_send_header_to(amqp_connection_state_t state,
			amqp_output_fn_t fn,
			void *context)
{
  return fn(context, header(), 8);
}

static amqp_bytes_t sasl_method_name(amqp_sasl_method_enum method) {
  switch (method) {
    case AMQP_SASL_METHOD_PLAIN: return (amqp_bytes_t) {.len = 5, .bytes = "PLAIN"};
    default:
      amqp_assert(0, "Invalid SASL method: %d", (int) method);
  }
  abort(); /* unreachable */
}

static amqp_bytes_t sasl_response(amqp_pool_t *pool,
				  amqp_sasl_method_enum method,
				  va_list args)
{
  amqp_bytes_t response;

  switch (method) {
    case AMQP_SASL_METHOD_PLAIN: {
      char *username = va_arg(args, char *);
      size_t username_len = strlen(username);
      char *password = va_arg(args, char *);
      size_t password_len = strlen(password);
      amqp_pool_alloc_bytes(pool, strlen(username) + strlen(password) + 2, &response);
      *BUF_AT(response, 0) = 0;
      memcpy(((char *) response.bytes) + 1, username, username_len);
      *BUF_AT(response, username_len + 1) = 0;
      memcpy(((char *) response.bytes) + username_len + 2, password, password_len);
      break;
    }
    default:
      amqp_assert(0, "Invalid SASL method: %d", (int) method);
  }

  return response;
}

amqp_boolean_t amqp_frames_enqueued(amqp_connection_state_t state) {
  return (state->first_queued_frame != NULL);
}

/*
 * Check to see if we have data in our buffer. If this returns 1, we
 * will avoid an immediate blocking read in amqp_simple_wait_frame.
 */
amqp_boolean_t amqp_data_in_buffer(amqp_connection_state_t state) {
  return (state->sock_inbound_offset < state->sock_inbound_limit);
}

static int wait_frame_inner(amqp_connection_state_t state,
			    amqp_frame_t *decoded_frame)
{
  while (1) {
    int result;

    while (amqp_data_in_buffer(state)) {
      amqp_bytes_t buffer;
      buffer.len = state->sock_inbound_limit - state->sock_inbound_offset;
      buffer.bytes = ((char *) state->sock_inbound_buffer.bytes) + state->sock_inbound_offset;
      AMQP_CHECK_RESULT((result = amqp_handle_input(state, buffer, decoded_frame)));
      state->sock_inbound_offset += result;

      if (decoded_frame->frame_type != 0) {
	/* Complete frame was read. Return it. */
	return 1;
      }

      /* Incomplete or ignored frame. Keep processing input. */
      assert(result != 0);
    }	

    result = read(state->sockfd,
		  state->sock_inbound_buffer.bytes,
		  state->sock_inbound_buffer.len);
    if (result < 0) {
      return -errno;
    }
    if (result == 0) {
      /* EOF. */
      return 0;
    }

    state->sock_inbound_limit = result;
    state->sock_inbound_offset = 0;
  }
}

int amqp_simple_wait_frame(amqp_connection_state_t state,
			   amqp_frame_t *decoded_frame)
{
  if (state->first_queued_frame != NULL) {
    amqp_frame_t *f = (amqp_frame_t *) state->first_queued_frame->data;
    state->first_queued_frame = state->first_queued_frame->next;
    if (state->first_queued_frame == NULL) {
      state->last_queued_frame = NULL;
    }
    *decoded_frame = *f;
    return 1;
  } else {
    return wait_frame_inner(state, decoded_frame);
  }
}

int amqp_simple_wait_method(amqp_connection_state_t state,
			    amqp_channel_t expected_channel,
			    amqp_method_number_t expected_method,
			    amqp_method_t *output)
{
  amqp_frame_t frame;

  AMQP_CHECK_EOF_RESULT(amqp_simple_wait_frame(state, &frame));
  amqp_assert(frame.channel == expected_channel,
	      "Expected 0x%08X method frame on channel %d, got frame on channel %d",
	      expected_method,
	      expected_channel,
	      frame.channel);
  amqp_assert(frame.frame_type == AMQP_FRAME_METHOD,
	      "Expected 0x%08X method frame on channel %d, got frame type %d",
	      expected_method,
	      expected_channel,
	      frame.frame_type);
  amqp_assert(frame.payload.method.id == expected_method,
	      "Expected method ID 0x%08X on channel %d, got ID 0x%08X",
	      expected_method,
	      expected_channel,
	      frame.payload.method.id);
  *output = frame.payload.method;
  return 1;
}

int amqp_send_method(amqp_connection_state_t state,
		     amqp_channel_t channel,
		     amqp_method_number_t id,
		     void *decoded)
{
  amqp_frame_t frame;

  frame.frame_type = AMQP_FRAME_METHOD;
  frame.channel = channel;
  frame.payload.method.id = id;
  frame.payload.method.decoded = decoded;
  return amqp_send_frame(state, &frame);
}

static int amqp_id_in_reply_list( amqp_method_number_t expected, amqp_method_number_t *list )
{
  while ( *list != 0 ) {
    if ( *list == expected ) return 1;
    list++;
  }
  return 0;
}

amqp_rpc_reply_t amqp_simple_rpc(amqp_connection_state_t state,
				 amqp_channel_t channel,
				 amqp_method_number_t request_id,
				 amqp_method_number_t *expected_reply_ids,
				 void *decoded_request_method)
{
  int status;
  amqp_rpc_reply_t result;

  memset(&result, 0, sizeof(result));

  status = amqp_send_method(state, channel, request_id, decoded_request_method);
  if (status < 0) {
    result.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
    result.library_errno = -status;
    return result;
  }

  {
    amqp_frame_t frame;

  retry:
    status = wait_frame_inner(state, &frame);
    if (status <= 0) {
      result.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
      result.library_errno = -status;
      return result;
    }

    /*
     * We store the frame for later processing unless it's something
     * that directly affects us here, namely a method frame that is
     * either
     *  - on the channel we want, and of the expected type, or
     *  - on the channel we want, and a channel.close frame, or
     *  - on channel zero, and a connection.close frame.
     */
    if (!( (frame.frame_type == AMQP_FRAME_METHOD) &&
	   (   ((frame.channel == channel) &&
		((amqp_id_in_reply_list(frame.payload.method.id, expected_reply_ids)) ||
		 (frame.payload.method.id == AMQP_CHANNEL_CLOSE_METHOD)))
	    ||
	       ((frame.channel == 0) &&
		(frame.payload.method.id == AMQP_CONNECTION_CLOSE_METHOD))   ) ))
    {	     
      amqp_frame_t *frame_copy = amqp_pool_alloc(&state->decoding_pool, sizeof(amqp_frame_t));
      amqp_link_t *link = amqp_pool_alloc(&state->decoding_pool, sizeof(amqp_link_t));

      *frame_copy = frame;

      link->next = NULL;
      link->data = frame_copy;

      if (state->last_queued_frame == NULL) {
	state->first_queued_frame = link;
      } else {
	state->last_queued_frame->next = link;
      }
      state->last_queued_frame = link;

      goto retry;
    }

    result.reply_type = (amqp_id_in_reply_list(frame.payload.method.id, expected_reply_ids))
      ? AMQP_RESPONSE_NORMAL
      : AMQP_RESPONSE_SERVER_EXCEPTION;

    result.reply = frame.payload.method;
    return result;
  }
}

static int amqp_login_inner(amqp_connection_state_t state,
			    int channel_max,
			    int frame_max,
			    int heartbeat,
			    amqp_sasl_method_enum sasl_method,
			    va_list vl)
{
  amqp_method_t method;
  uint32_t server_frame_max;
  uint16_t server_channel_max;
  uint16_t server_heartbeat;

  amqp_send_header(state);

  AMQP_CHECK_EOF_RESULT(amqp_simple_wait_method(state, 0, AMQP_CONNECTION_START_METHOD, &method));
  {
    amqp_connection_start_t *s = (amqp_connection_start_t *) method.decoded;
    if ((s->version_major != AMQP_PROTOCOL_VERSION_MAJOR) ||
	(s->version_minor != AMQP_PROTOCOL_VERSION_MINOR)) {
      return -EPROTOTYPE;
    }

    /* TODO: check that our chosen SASL mechanism is in the list of
       acceptable mechanisms. Or even let the application choose from
       the list! */
  }

  {
    amqp_bytes_t response_bytes = sasl_response(&state->decoding_pool, sasl_method, vl);
    amqp_connection_start_ok_t s =
      (amqp_connection_start_ok_t) {
        .client_properties = {.num_entries = 0, .entries = NULL},
	.mechanism = sasl_method_name(sasl_method),
	.response = response_bytes,
	.locale = {.len = 5, .bytes = "en_US"}
      };
    AMQP_CHECK_RESULT(amqp_send_method(state, 0, AMQP_CONNECTION_START_OK_METHOD, &s));
  }

  amqp_release_buffers(state);

  AMQP_CHECK_EOF_RESULT(amqp_simple_wait_method(state, 0, AMQP_CONNECTION_TUNE_METHOD, &method));
  {
    amqp_connection_tune_t *s = (amqp_connection_tune_t *) method.decoded;
    server_channel_max = s->channel_max;
    server_frame_max = s->frame_max;
    server_heartbeat = s->heartbeat;
  }

  if (server_channel_max != 0 && server_channel_max < channel_max) {
    channel_max = server_channel_max;
  }

  if (server_frame_max != 0 && server_frame_max < frame_max) {
    frame_max = server_frame_max;
  }

  if (server_heartbeat != 0 && server_heartbeat < heartbeat) {
    heartbeat = server_heartbeat;
  }

  AMQP_CHECK_RESULT(amqp_tune_connection(state, channel_max, frame_max, heartbeat));

  {
    amqp_connection_tune_ok_t s =
      (amqp_connection_tune_ok_t) {
        .channel_max = channel_max,
	.frame_max = frame_max,
	.heartbeat = heartbeat
      };
    AMQP_CHECK_RESULT(amqp_send_method(state, 0, AMQP_CONNECTION_TUNE_OK_METHOD, &s));
  }

  amqp_release_buffers(state);

  return 1;
}

amqp_rpc_reply_t amqp_login(amqp_connection_state_t state,
			    char const *vhost,
			    int channel_max,
			    int frame_max,
			    int heartbeat,
			    amqp_sasl_method_enum sasl_method,
			    ...)
{
  va_list vl;
  amqp_rpc_reply_t result;

  va_start(vl, sasl_method);

  amqp_login_inner(state, channel_max, frame_max, heartbeat, sasl_method, vl);

  {
    amqp_connection_open_t s =
      (amqp_connection_open_t) {
        .virtual_host = amqp_cstring_bytes(vhost),
	.capabilities = {.len = 0, .bytes = NULL},
	.insist = 1
      };
    amqp_method_number_t replies[] = { AMQP_CONNECTION_OPEN_OK_METHOD, 0 };
    result = amqp_simple_rpc(state,
			     0,
			     AMQP_CONNECTION_OPEN_METHOD,
			     (amqp_method_number_t *) &replies,
			     &s);
    if (result.reply_type != AMQP_RESPONSE_NORMAL) {
      return result;
    }
  }
  amqp_maybe_release_buffers(state);

  va_end(vl);

  result.reply_type = AMQP_RESPONSE_NORMAL;
  result.reply.id = 0;
  result.reply.decoded = NULL;
  result.library_errno = 0;
  return result;
}
