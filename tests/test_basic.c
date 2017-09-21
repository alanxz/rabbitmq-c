/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
/*
 * Copyright 2014 Michael Steinert
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "amqp.h"
#include "amqp_tcp_socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/time.h>
#endif

#define assert(x)                                    \
  {                                                  \
    if (!(x)) {                                      \
      fprintf(stderr, "Assertion failed: %s\n", #x); \
      abort();                                       \
    }                                                \
  }

static const int fixed_channel_id = 1;
static const char test_queue_name[] = "test_queue";

amqp_connection_state_t setup_connection_and_channel(void) {
  amqp_connection_state_t connection_state_ = amqp_new_connection();

  amqp_socket_t *socket = amqp_tcp_socket_new(connection_state_);
  assert(socket);

  int rc = amqp_socket_open(socket, "localhost", AMQP_PROTOCOL_PORT);
  assert(rc == AMQP_STATUS_OK);

  amqp_rpc_reply_t rpc_reply = amqp_login_with_properties(
      connection_state_, "/", 1, AMQP_DEFAULT_FRAME_SIZE, 0, NULL,
      AMQP_SASL_METHOD_PLAIN, "guest", "guest");
  assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);

  amqp_channel_open_ok_t *res =
      amqp_channel_open(connection_state_, fixed_channel_id);
  assert(res != NULL);

  return connection_state_;
}

void close_channel_and_connection(amqp_connection_state_t connection_state_) {
  amqp_rpc_reply_t rpc_reply =
      amqp_channel_close(connection_state_, 1, AMQP_REPLY_SUCCESS);
  assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);

  rpc_reply = amqp_connection_close(connection_state_, AMQP_REPLY_SUCCESS);
  assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);

  int rc = amqp_destroy_connection(connection_state_);
  assert(rc == AMQP_STATUS_OK);
}

void basic_publish(amqp_connection_state_t *connectionState_,
                   const char *message_) {
  amqp_bytes_t message_bytes;
  message_bytes.len = strlen(message_);
  message_bytes.bytes = (char *)message_;

  amqp_basic_properties_t properties;
  properties._flags = 0;

  properties._flags |= AMQP_BASIC_DELIVERY_MODE_FLAG;
  properties.delivery_mode = AMQP_DELIVERY_NONPERSISTENT;

  int retval = amqp_basic_publish(
      *connectionState_, fixed_channel_id, amqp_cstring_bytes(""),
      amqp_cstring_bytes(test_queue_name),
      /* mandatory=*/1,
      /* immediate=*/0,  // RabbitMQ 3.x does not support the "immediate" flag
                         // according to
                         // https://www.rabbitmq.com/specification.html
      &properties, message_bytes);

  assert(retval == 0);
}

void queue_declare(amqp_connection_state_t *connection_state_,
                   const char *queue_name_) {
  amqp_queue_declare_ok_t *res = amqp_queue_declare(
      *connection_state_, fixed_channel_id, amqp_cstring_bytes(queue_name_),
      /*passive*/ 0,
      /*durable*/ 0,
      /*exclusive*/ 0,
      /*auto_delete*/ 1, amqp_empty_table);
  assert(res != NULL);
}

void receive_body_frames(amqp_connection_state_t *connection_state_,
                         char *body_, uint64_t body_size_) {
  char *body_begin = body_;
  uint64_t body_read, body_left;
  for (body_read = 0, body_left = body_size_; body_read < body_size_;) {
    amqp_frame_t body_frame;
    int rc = amqp_simple_wait_frame(*connection_state_, &body_frame);
    assert(rc == AMQP_STATUS_OK);

    assert(body_frame.frame_type == AMQP_FRAME_BODY);

    amqp_bytes_t *body_fragment = &body_frame.payload.body_fragment;

    assert(body_fragment->len <= body_left);

    memcpy(body_begin + body_read, body_fragment->bytes, body_fragment->len);
    body_read += body_fragment->len;
    body_left -= body_fragment->len;
  }
}

char *basic_get(amqp_connection_state_t *connection_state_,
                const char *queue_name_, uint64_t *out_body_size_) {
  amqp_rpc_reply_t rpc_reply;
  do {
    rpc_reply = amqp_basic_get(*connection_state_, fixed_channel_id,
                               amqp_cstring_bytes(queue_name_), /*no_ack*/ 1);
  } while (rpc_reply.reply_type == AMQP_RESPONSE_NORMAL &&
           rpc_reply.reply.id == AMQP_BASIC_GET_EMPTY_METHOD);

  assert(rpc_reply.reply.id == AMQP_BASIC_GET_OK_METHOD);

  amqp_frame_t header_frame;
  int rc = amqp_simple_wait_frame(*connection_state_, &header_frame);
  assert(rc == AMQP_STATUS_OK);

  assert(header_frame.frame_type == AMQP_FRAME_HEADER);

  *out_body_size_ = header_frame.payload.properties.body_size;

  // message body cannot be from the pool, body is freed in envelope_destroy
  // with amqp_bytes_free
  char *body = malloc(*out_body_size_);
  if (*out_body_size_) {
    receive_body_frames(connection_state_, body, *out_body_size_);
  }
  return body;
}

void publish_and_basic_get_message(const char *msg_to_publish) {
  amqp_connection_state_t connection_state = setup_connection_and_channel();

  queue_declare(&connection_state, test_queue_name);
  basic_publish(&connection_state, msg_to_publish);

  uint64_t body_size;
  char *msg = basic_get(&connection_state, test_queue_name, &body_size);

  assert(body_size == strlen(msg_to_publish));
  assert(strncmp(msg_to_publish, msg, body_size) == 0);
  free(msg);

  close_channel_and_connection(connection_state);
}

char *consume_message(amqp_connection_state_t *connection_state_,
                      const char *queue_name_, uint64_t *out_body_size_) {
  amqp_basic_consume_ok_t *result =
      amqp_basic_consume(*connection_state_, fixed_channel_id,
                         amqp_cstring_bytes(queue_name_), amqp_empty_bytes,
                         /*no_local*/ 0,
                         /*no_ack*/ 1,
                         /*exclusive*/ 0, amqp_empty_table);
  assert(result != NULL);

  amqp_envelope_t envelope;
  struct timeval timeout = {0, 250};
  amqp_rpc_reply_t rpc_reply =
      amqp_consume_message(*connection_state_, &envelope, &timeout, 0);
  assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);

  *out_body_size_ = envelope.message.body.len;
  char *body = malloc(*out_body_size_);
  if (*out_body_size_) {
    memcpy(body, envelope.message.body.bytes, *out_body_size_);
  }

  amqp_destroy_envelope(&envelope);
  return body;
}

void publish_and_consume_message(const char *msg_to_publish) {
  amqp_connection_state_t connection_state = setup_connection_and_channel();

  queue_declare(&connection_state, test_queue_name);
  basic_publish(&connection_state, msg_to_publish);

  uint64_t body_size;
  char *msg = consume_message(&connection_state, test_queue_name, &body_size);

  assert(body_size == strlen(msg_to_publish));
  assert(strncmp(msg_to_publish, msg, body_size) == 0);
  free(msg);

  close_channel_and_connection(connection_state);
}

int main(void) {
  publish_and_basic_get_message("");
  publish_and_basic_get_message("TEST");

  publish_and_consume_message("");
  publish_and_consume_message("TEST");

  return 0;
}
