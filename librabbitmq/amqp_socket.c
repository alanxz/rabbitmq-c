/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
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

#include "amqp_private.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>

int amqp_open_socket(char const *hostname,
		     int portnumber)
{
  struct addrinfo hint;
  struct addrinfo *address_list;
  struct addrinfo *addr;
  char portnumber_string[33];
  int sockfd = -1;
  int last_error = 0;
  int one = 1; /* for setsockopt */

  if (0 != (last_error = amqp_socket_init()))
    return last_error;

  memset(&hint, 0, sizeof(hint));
  hint.ai_family = PF_UNSPEC; /* PF_INET or PF_INET6 */
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_protocol = IPPROTO_TCP;

  (void)sprintf(portnumber_string, "%d", portnumber);

  last_error = getaddrinfo(hostname, portnumber_string, &hint, &address_list);

  if (last_error != 0)
  {
    return -ERROR_GETHOSTBYNAME_FAILED;
  }

  for (addr = address_list; addr; addr = addr->ai_next)
  {
    /*
      This cast is to squash warnings on Win64, see:
      http://stackoverflow.com/questions/1953639/is-it-safe-to-cast-socket-to-int-under-win64
    */
    sockfd = (int)socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (-1 == sockfd)
    {
      last_error = -amqp_socket_error();
      continue;
    }
#ifdef DISABLE_SIGPIPE_WITH_SETSOCKOPT
    if (0 != amqp_socket_setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one)))
    {
      last_error = -amqp_socket_error();
      amqp_socket_close(sockfd);
      continue;
    }
#endif /* DISABLE_SIGPIPE_WITH_SETSOCKOPT */
    if (0 != amqp_socket_setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one))
        || 0 != connect(sockfd, addr->ai_addr, addr->ai_addrlen))
    {
      last_error = -amqp_socket_error();
      amqp_socket_close(sockfd);
      continue;
    }
    else
    {
      last_error = 0;
      break;
    }
  }

  freeaddrinfo(address_list);
  if (last_error != 0)
  {
    return last_error;
  }
  
  return sockfd;
}

int amqp_send_header(amqp_connection_state_t state) {
  static const uint8_t header[8] = { 'A', 'M', 'Q', 'P', 0,
				     AMQP_PROTOCOL_VERSION_MAJOR,
				     AMQP_PROTOCOL_VERSION_MINOR,
				     AMQP_PROTOCOL_VERSION_REVISION };
  return send(state->sockfd, (void *)header, 8, MSG_NOSIGNAL);
}

static amqp_bytes_t sasl_method_name(amqp_sasl_method_enum method) {
  amqp_bytes_t res;

  switch (method) {
  case AMQP_SASL_METHOD_PLAIN:
    res.bytes = "PLAIN";
    res.len = 5;
    break;

  default:
    amqp_abort("Invalid SASL method: %d", (int) method);
  }

  return res;
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
      char *response_buf;

      amqp_pool_alloc_bytes(pool, strlen(username) + strlen(password) + 2, &response);
      if (response.bytes == NULL)
	/* We never request a zero-length block, because of the +2
	   above, so a NULL here really is ENOMEM. */
	return response;

      response_buf = response.bytes;
      response_buf[0] = 0;
      memcpy(response_buf + 1, username, username_len);
      response_buf[username_len + 1] = 0;
      memcpy(response_buf + username_len + 2, password, password_len);
      break;
    }
    default:
      amqp_abort("Invalid SASL method: %d", (int) method);
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
    int res;

    while (amqp_data_in_buffer(state)) {
      amqp_bytes_t buffer;
      buffer.len = state->sock_inbound_limit - state->sock_inbound_offset;
      buffer.bytes = ((char *) state->sock_inbound_buffer.bytes) + state->sock_inbound_offset;

      res = amqp_handle_input(state, buffer, decoded_frame);
      if (res < 0)
	return res;

      state->sock_inbound_offset += res;

      if (decoded_frame->frame_type != 0)
	/* Complete frame was read. Return it. */
	return 0;

      /* Incomplete or ignored frame. Keep processing input. */
      assert(res != 0);
    }

    res = recv(state->sockfd, state->sock_inbound_buffer.bytes,
		  state->sock_inbound_buffer.len, 0);
    if (res <= 0) {
      if (res == 0)
	return -ERROR_CONNECTION_CLOSED;
      else
	return -amqp_socket_error();
    }

    state->sock_inbound_limit = res;
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
    return 0;
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
  int res = amqp_simple_wait_frame(state, &frame);
  if (res < 0)
    return res;

  if (frame.channel != expected_channel)
    amqp_abort("Expected 0x%08X method frame on channel %d, got frame on channel %d",
	       expected_method,
	       expected_channel,
	       frame.channel);
  if (frame.frame_type != AMQP_FRAME_METHOD)
    amqp_abort("Expected 0x%08X method frame on channel %d, got frame type %d",
	       expected_method,
	       expected_channel,
	       frame.frame_type);
  if (frame.payload.method.id != expected_method)
    amqp_abort("Expected method ID 0x%08X on channel %d, got ID 0x%08X",
	       expected_method,
	       expected_channel,
	       frame.payload.method.id);
  *output = frame.payload.method;
  return 0;
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
    result.library_error = -status;
    return result;
  }

  {
    amqp_frame_t frame;

  retry:
    status = wait_frame_inner(state, &frame);
    if (status < 0) {
      result.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
      result.library_error = -status;
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

      if (frame_copy == NULL || link == NULL) {
	result.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
	result.library_error = ERROR_NO_MEMORY;
	return result;
      }

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

void *amqp_simple_rpc_decoded(amqp_connection_state_t state,
			      amqp_channel_t channel,
			      amqp_method_number_t request_id,
			      amqp_method_number_t reply_id,
			      void *decoded_request_method)
{
  amqp_method_number_t replies[2];

  replies[0] = reply_id;
  replies[1] = 0;

  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  request_id, replies,
						  decoded_request_method);
  if (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL)
    return state->most_recent_api_result.reply.decoded;
  else
    return NULL;
}

amqp_rpc_reply_t amqp_get_rpc_reply(amqp_connection_state_t state)
{
  return state->most_recent_api_result;
}


static int amqp_login_inner(amqp_connection_state_t state,
			    int channel_max,
			    int frame_max,
			    int heartbeat,
			    amqp_sasl_method_enum sasl_method,
			    va_list vl)
{
  int res;
  amqp_method_t method;
  int server_frame_max;
  uint16_t server_channel_max;
  uint16_t server_heartbeat;

  amqp_send_header(state);

  res = amqp_simple_wait_method(state, 0, AMQP_CONNECTION_START_METHOD,
				&method);
  if (res < 0)
    return res;

  {
    amqp_connection_start_t *s = (amqp_connection_start_t *) method.decoded;
    if ((s->version_major != AMQP_PROTOCOL_VERSION_MAJOR) ||
	(s->version_minor != AMQP_PROTOCOL_VERSION_MINOR)) {
      return -ERROR_INCOMPATIBLE_AMQP_VERSION;
    }

    /* TODO: check that our chosen SASL mechanism is in the list of
       acceptable mechanisms. Or even let the application choose from
       the list! */
  }

  {
    amqp_table_entry_t properties[2];
    amqp_connection_start_ok_t s;
    amqp_bytes_t response_bytes = sasl_response(&state->decoding_pool,
						sasl_method, vl);

    if (response_bytes.bytes == NULL)
      return -ERROR_NO_MEMORY;

    properties[0].key = amqp_cstring_bytes("product");
    properties[0].value.kind = AMQP_FIELD_KIND_UTF8;
    properties[0].value.value.bytes
      = amqp_cstring_bytes("rabbitmq-c");

    properties[1].key = amqp_cstring_bytes("information");
    properties[1].value.kind = AMQP_FIELD_KIND_UTF8;
    properties[1].value.value.bytes
      = amqp_cstring_bytes("See https://github.com/alanxz/rabbitmq-c");

    s.client_properties.num_entries = 2;
    s.client_properties.entries = properties;
    s.mechanism = sasl_method_name(sasl_method);
    s.response = response_bytes;
    s.locale.bytes = "en_US";
    s.locale.len = 5;

    res = amqp_send_method(state, 0, AMQP_CONNECTION_START_OK_METHOD, &s);
    if (res < 0)
      return res;
  }

  amqp_release_buffers(state);

  res = amqp_simple_wait_method(state, 0, AMQP_CONNECTION_TUNE_METHOD,
				&method);
  if (res < 0)
    return res;

  {
    amqp_connection_tune_t *s = (amqp_connection_tune_t *) method.decoded;
    server_channel_max = s->channel_max;
    server_frame_max = s->frame_max;
    server_heartbeat = s->heartbeat;
  }

  if (server_channel_max != 0 && server_channel_max < channel_max)
    channel_max = server_channel_max;

  if (server_frame_max != 0 && server_frame_max < frame_max)
    frame_max = server_frame_max;

  if (server_heartbeat != 0 && server_heartbeat < heartbeat)
    heartbeat = server_heartbeat;

  res = amqp_tune_connection(state, channel_max, frame_max, heartbeat);
  if (res < 0)
    return res;

  {
    amqp_connection_tune_ok_t s;
    s.frame_max = frame_max;
    s.channel_max = channel_max;
    s.heartbeat = heartbeat;

    res = amqp_send_method(state, 0, AMQP_CONNECTION_TUNE_OK_METHOD, &s);
    if (res < 0)
      return res;
  }

  amqp_release_buffers(state);

  return 0;
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
  int status;

  va_start(vl, sasl_method);

  status = amqp_login_inner(state, channel_max, frame_max, heartbeat, sasl_method, vl);
  if (status < 0) {
    result.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
    result.reply.id = 0;
    result.reply.decoded = NULL;
    result.library_error = -status;
    return result;
  }

  {
    amqp_method_number_t replies[] = { AMQP_CONNECTION_OPEN_OK_METHOD, 0 };
    amqp_connection_open_t s;
    s.virtual_host = amqp_cstring_bytes(vhost);
    s.capabilities.len = 0;
    s.capabilities.bytes = NULL;
    s.insist = 1;

    result = amqp_simple_rpc(state,
			     0,
			     AMQP_CONNECTION_OPEN_METHOD,
			     (amqp_method_number_t *) &replies,
			     &s);
    if (result.reply_type != AMQP_RESPONSE_NORMAL)
      return result;
  }
  amqp_maybe_release_buffers(state);

  va_end(vl);

  result.reply_type = AMQP_RESPONSE_NORMAL;
  result.reply.id = 0;
  result.reply.decoded = NULL;
  result.library_error = 0;
  return result;
}
