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
#include <stdarg.h>

#include "amqp.h"
#include "amqp_framing.h"
#include "amqp_private.h"

#include <assert.h>

static const char *client_error_strings[ERROR_MAX] = {
  "could not allocate memory", /* ERROR_NO_MEMORY */
  "received bad AMQP data", /* ERROR_BAD_AQMP_DATA */
  "unknown AMQP class id", /* ERROR_UNKOWN_CLASS */
  "unknown AMQP method id", /* ERROR_UNKOWN_METHOD */
  "unknown host", /* ERROR_GETHOSTBYNAME_FAILED */
  "incompatible AMQP version", /* ERROR_INCOMPATIBLE_AMQP_VERSION */
  "connection closed unexpectedly", /* ERROR_CONNECTION_CLOSED */
};

/* strdup is not in ISO C90! */
static inline char *strdup(const char *str)
{
  return strcpy(malloc(strlen(str) + 1),str);
}

char *amqp_error_string(int err)
{
  const char *str;
  int category = (err & ERROR_CATEGORY_MASK);
  err = (err & ~ERROR_CATEGORY_MASK);

  switch (category) {
  case ERROR_CATEGORY_CLIENT:
    if (err < 1 || err > ERROR_MAX)
      str = "(undefined librabbitmq error)";
    else
      str = client_error_strings[err - 1];
    break;

  case ERROR_CATEGORY_OS:
    return amqp_os_error_string(err);
    
  default:
    str = "(undefined error category)";
  }

  return strdup(str);
}

void amqp_abort(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	abort();
}

const amqp_bytes_t amqp_empty_bytes = { 0, NULL };
const amqp_table_t amqp_empty_table = { 0, NULL };
const amqp_array_t amqp_empty_array = { 0, NULL };

#define RPC_REPLY(replytype)						\
  (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL	\
   ? (replytype *) state->most_recent_api_result.reply.decoded		\
   : NULL)

amqp_channel_open_ok_t *amqp_channel_open(amqp_connection_state_t state,
					  amqp_channel_t channel)
{
  amqp_method_number_t replies[2] = { AMQP_CHANNEL_OPEN_OK_METHOD, 0};
  amqp_channel_open_t req;
  req.out_of_band.bytes = NULL;
  req.out_of_band.len = 0;

  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  AMQP_CHANNEL_OPEN_METHOD,
						  replies, &req);
  if (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL)
    return state->most_recent_api_result.reply.decoded;
  else
    return NULL;
}

int amqp_basic_publish(amqp_connection_state_t state,
		       amqp_channel_t channel,
		       amqp_bytes_t exchange,
		       amqp_bytes_t routing_key,
		       amqp_boolean_t mandatory,
		       amqp_boolean_t immediate,
		       amqp_basic_properties_t const *properties,
		       amqp_bytes_t body)
{
  amqp_frame_t f;
  size_t body_offset;
  size_t usable_body_payload_size = state->frame_max - (HEADER_SIZE + FOOTER_SIZE);
  int res;

  amqp_basic_publish_t m;
  amqp_basic_properties_t default_properties;

  m.exchange = exchange;
  m.routing_key = routing_key;
  m.mandatory = mandatory;
  m.immediate = immediate;

  res = amqp_send_method(state, channel, AMQP_BASIC_PUBLISH_METHOD, &m);
  if (res < 0)
    return res;

  if (properties == NULL) {
    memset(&default_properties, 0, sizeof(default_properties));
    properties = &default_properties;
  }

  f.frame_type = AMQP_FRAME_HEADER;
  f.channel = channel;
  f.payload.properties.class_id = AMQP_BASIC_CLASS;
  f.payload.properties.body_size = body.len;
  f.payload.properties.decoded = (void *) properties;

  res = amqp_send_frame(state, &f);
  if (res < 0)
    return res;

  body_offset = 0;
  while (1) {
    int remaining = body.len - body_offset;
    assert(remaining >= 0);

    if (remaining == 0)
      break;

    f.frame_type = AMQP_FRAME_BODY;
    f.channel = channel;
    f.payload.body_fragment.bytes = amqp_offset(body.bytes, body_offset);
    if (remaining >= usable_body_payload_size) {
      f.payload.body_fragment.len = usable_body_payload_size;
    } else {
      f.payload.body_fragment.len = remaining;
    }

    body_offset += f.payload.body_fragment.len;
    res = amqp_send_frame(state, &f);
    if (res < 0)
      return res;
  }

  return 0;
}

amqp_rpc_reply_t amqp_channel_close(amqp_connection_state_t state,
				    amqp_channel_t channel,
				    int code)
{
  char codestr[13];
  amqp_method_number_t replies[2] = { AMQP_CHANNEL_CLOSE_OK_METHOD, 0};
  amqp_channel_close_t req;

  req.reply_code = code;
  req.reply_text.bytes = codestr;
  req.reply_text.len = sprintf(codestr, "%d", code);
  req.class_id = 0;
  req.method_id = 0;

  return amqp_simple_rpc(state, channel, AMQP_CHANNEL_CLOSE_METHOD,
			 replies, &req);
}

amqp_rpc_reply_t amqp_connection_close(amqp_connection_state_t state,
				       int code)
{
  char codestr[13];
  amqp_method_number_t replies[2] = { AMQP_CONNECTION_CLOSE_OK_METHOD, 0};
  amqp_channel_close_t req;

  req.reply_code = code;
  req.reply_text.bytes = codestr;
  req.reply_text.len = sprintf(codestr, "%d", code);
  req.class_id = 0;
  req.method_id = 0;

  return amqp_simple_rpc(state, 0, AMQP_CONNECTION_CLOSE_METHOD,
			 replies, &req);
}

amqp_exchange_declare_ok_t *amqp_exchange_declare(amqp_connection_state_t state,
						  amqp_channel_t channel,
						  amqp_bytes_t exchange,
						  amqp_bytes_t type,
						  amqp_boolean_t passive,
						  amqp_boolean_t durable,
						  amqp_table_t arguments)
{
  amqp_method_number_t replies[2] = { AMQP_EXCHANGE_DECLARE_OK_METHOD, 0};
  amqp_exchange_declare_t req;
  req.exchange = exchange;
  req.type = type;
  req.passive = passive;
  req.durable = durable;
  req.auto_delete = 0;
  req.internal = 0;
  req.nowait = 0;
  req.arguments = arguments;

  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  AMQP_EXCHANGE_DECLARE_METHOD,
						  replies, &req);
  if (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL)
    return state->most_recent_api_result.reply.decoded;
  else
    return NULL;
}

amqp_queue_declare_ok_t *amqp_queue_declare(amqp_connection_state_t state,
					    amqp_channel_t channel,
					    amqp_bytes_t queue,
					    amqp_boolean_t passive,
					    amqp_boolean_t durable,
					    amqp_boolean_t exclusive,
					    amqp_boolean_t auto_delete,
					    amqp_table_t arguments)
{
  amqp_method_number_t replies[2] = { AMQP_QUEUE_DECLARE_OK_METHOD, 0};
  amqp_queue_declare_t req;
  req.queue = queue;
  req.passive = passive;
  req.durable = durable;
  req.exclusive = exclusive;
  req.auto_delete = auto_delete;
  req.nowait = 0;
  req.arguments = arguments;

  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  AMQP_QUEUE_DECLARE_METHOD,
						  replies, &req);
  if (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL)
    return state->most_recent_api_result.reply.decoded;
  else
    return NULL;
}

amqp_queue_delete_ok_t *amqp_queue_delete(amqp_connection_state_t state,
					  amqp_channel_t channel,
					  amqp_bytes_t queue,
					  amqp_boolean_t if_unused,
					  amqp_boolean_t if_empty)
{
  amqp_method_number_t replies[2] = { AMQP_QUEUE_DELETE_OK_METHOD, 0};
  amqp_queue_delete_t req;
  req.queue = queue;
  req.if_unused = if_unused;
  req.if_empty = if_empty;
  req.nowait = 0;

  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  AMQP_QUEUE_DELETE_METHOD,
						  replies, &req);
  if (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL)
    return state->most_recent_api_result.reply.decoded;
  else
    return NULL;
}

amqp_queue_bind_ok_t *amqp_queue_bind(amqp_connection_state_t state,
				      amqp_channel_t channel,
				      amqp_bytes_t queue,
				      amqp_bytes_t exchange,
				      amqp_bytes_t routing_key,
				      amqp_table_t arguments)
{
  amqp_method_number_t replies[2] = { AMQP_QUEUE_BIND_OK_METHOD, 0};
  amqp_queue_bind_t req;
  req.ticket = 0;
  req.queue = queue;
  req.exchange = exchange;
  req.routing_key = routing_key;
  req.nowait = 0;
  req.arguments = arguments;

  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  AMQP_QUEUE_BIND_METHOD,
						  replies, &req);
  if (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL)
    return state->most_recent_api_result.reply.decoded;
  else
    return NULL;
}

amqp_queue_unbind_ok_t *amqp_queue_unbind(amqp_connection_state_t state,
					  amqp_channel_t channel,
					  amqp_bytes_t queue,
					  amqp_bytes_t exchange,
					  amqp_bytes_t routing_key,
					  amqp_table_t arguments)
{
  amqp_method_number_t replies[2] = { AMQP_QUEUE_UNBIND_OK_METHOD, 0};
  amqp_queue_unbind_t req;
  req.ticket = 0;
  req.queue = queue;
  req.exchange = exchange;
  req.routing_key = routing_key;
  req.arguments = arguments;

  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  AMQP_QUEUE_UNBIND_METHOD,
						  replies, &req);
  if (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL)
    return state->most_recent_api_result.reply.decoded;
  else
    return NULL;
}

amqp_basic_consume_ok_t *amqp_basic_consume(amqp_connection_state_t state,
					    amqp_channel_t channel,
					    amqp_bytes_t queue,
					    amqp_bytes_t consumer_tag,
					    amqp_boolean_t no_local,
					    amqp_boolean_t no_ack,
					    amqp_boolean_t exclusive,
					    amqp_table_t filter)
{
  amqp_method_number_t replies[2] = { AMQP_BASIC_CONSUME_OK_METHOD, 0};
  amqp_basic_consume_t req;
  req.ticket = 0;
  req.queue = queue;
  req.consumer_tag = consumer_tag;
  req.no_local = no_local;
  req.no_ack = no_ack;
  req.exclusive = exclusive;
  req.nowait = 0;
  req.arguments = filter;

  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  AMQP_BASIC_CONSUME_METHOD,
						  replies, &req);
  if (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL)
    return state->most_recent_api_result.reply.decoded;
  else
    return NULL;
}

int amqp_basic_ack(amqp_connection_state_t state,
		   amqp_channel_t channel,
		   uint64_t delivery_tag,
		   amqp_boolean_t multiple)
{
  amqp_basic_ack_t m;
  m.delivery_tag = delivery_tag;
  m.multiple = multiple;
  return amqp_send_method(state, channel, AMQP_BASIC_ACK_METHOD, &m);
}

amqp_queue_purge_ok_t *amqp_queue_purge(amqp_connection_state_t state,
					amqp_channel_t channel,
					amqp_bytes_t queue,
					amqp_boolean_t no_wait)
{
  amqp_method_number_t replies[2] = { AMQP_QUEUE_PURGE_OK_METHOD, 0};
  amqp_queue_purge_t req;
  req.ticket = 0;
  req.queue = queue;
  req.nowait = 0;

  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  AMQP_QUEUE_PURGE_METHOD,
						  replies, &req);
  if (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL)
    return state->most_recent_api_result.reply.decoded;
  else
    return NULL;
}

amqp_rpc_reply_t amqp_basic_get(amqp_connection_state_t state,
				amqp_channel_t channel,
				amqp_bytes_t queue,
				amqp_boolean_t no_ack)
{
  amqp_method_number_t replies[] = { AMQP_BASIC_GET_OK_METHOD,
				     AMQP_BASIC_GET_EMPTY_METHOD,
				     0 };
  amqp_basic_get_t req;
  req.ticket = 0;
  req.queue = queue;
  req.no_ack = no_ack;

  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  AMQP_BASIC_GET_METHOD,
						  replies, &req);
  return state->most_recent_api_result;
}

amqp_tx_select_ok_t *amqp_tx_select(amqp_connection_state_t state,
				    amqp_channel_t channel)
{
  amqp_method_number_t replies[2] = { AMQP_TX_SELECT_OK_METHOD, 0};
  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  AMQP_TX_SELECT_METHOD,
						  replies, NULL);
  if (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL)
    return state->most_recent_api_result.reply.decoded;
  else
    return NULL;
}

amqp_tx_commit_ok_t *amqp_tx_commit(amqp_connection_state_t state,
				    amqp_channel_t channel)
{
  amqp_method_number_t replies[2] = { AMQP_TX_COMMIT_OK_METHOD, 0};
  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  AMQP_TX_COMMIT_METHOD,
						  replies, NULL);
  if (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL)
    return state->most_recent_api_result.reply.decoded;
  else
    return NULL;
}

amqp_tx_rollback_ok_t *amqp_tx_rollback(amqp_connection_state_t state,
					amqp_channel_t channel)
{
  amqp_method_number_t replies[2] = { AMQP_TX_ROLLBACK_OK_METHOD, 0};
  state->most_recent_api_result = amqp_simple_rpc(state, channel,
						  AMQP_TX_ROLLBACK_METHOD,
						  replies, NULL);
  if (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL)
    return state->most_recent_api_result.reply.decoded;
  else
    return NULL;
}

amqp_rpc_reply_t amqp_get_rpc_reply(amqp_connection_state_t state)
{
  return state->most_recent_api_result;
}
