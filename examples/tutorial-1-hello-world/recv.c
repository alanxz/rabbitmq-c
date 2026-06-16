/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
 *
 * Portions created by Alan Antonuk are Copyright (c) 2017 Alan Antonuk.
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

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  const char* QUEUE_NAME = "hello";

  amqp_connection_state_t conn;
  amqp_socket_t *socket;
  int res;
  amqp_rpc_reply_t reply;
  amqp_channel_t chan;


  conn = amqp_new_connection();
  if (!conn) {
    fprintf(stderr, "failed to allocate connection object\n");
    return 1;
  }

  /* socket is owned by the conn */
  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    fprintf(stderr, "failed to allocate socket object\n");
    res = 2;
    goto cleanup1;
  }

  res = amqp_socket_open(socket, "localhost", AMQP_PROTOCOL_PORT);
  if (res != AMQP_STATUS_OK) {
    fprintf(stderr, "failed to open socket to broker: %s\n",
            amqp_error_string2(res));
    res = 3;
    goto cleanup1;
  }

  reply =
      amqp_login(conn, AMQP_DEFAULT_VHOST, AMQP_DEFAULT_MAX_CHANNELS,
                 AMQP_DEFAULT_FRAME_SIZE, AMQP_DEFAULT_HEARTBEAT,
                 AMQP_SASL_METHOD_PLAIN,
                 "guest", /* default guest account */
                 "guest"  /* default guest password */);
  if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
    fprintf(stderr, "failed login with broker\n");
    goto cleanup1;
  }

  chan = 1;
  if (!amqp_channel_open(conn, chan)) {
    fprintf(stderr, "failed to open channel\n");
    goto cleanup1;
  }

  {
    amqp_queue_declare_ok_t *queue = amqp_queue_declare(
        conn, chan, amqp_cstring_bytes(QUEUE_NAME),
        0, /* Not passive, actually declare the queue */
        0, /* Not durable, will not survive broker restart */
        0, /* Not exclusive to connection, may be used by other connections */
        0, /* Not automatically deleted upon disconnection */
        amqp_empty_table);
    if (!queue) {
      fprintf(stderr, "failed to declare queue: \n");
      abort();
    }
  }

  {
    const char* CONSUMER_NAME = "hello-consumer";
    amqp_basic_consume_ok_t *consumer = amqp_basic_consume(
        conn, chan, amqp_cstring_bytes(QUEUE_NAME),
        amqp_cstring_bytes(CONSUMER_NAME),
        0,  /* non-local, don't deliver messages published by this connection */
        1,  /* exclusive: only this consumer may access the queue */
        1,  /* no-ack: consider message ack'd upon delivery */
        amqp_empty_table);
    if (!consumer) {
      fprintf(stderr, "failed to start consumer: \n");
      abort();
    }

    printf(" [*] Waiting for messages. To exit press CTRL+C\n");
    for (;;) {
      amqp_envelope_t envelope;

      amqp_maybe_release_buffers(conn);
      reply = amqp_consume_message(conn, &envelope,
                                   NULL, /* no timeout waiting for a message */
                                   0 /* flags are unused currently */);
      if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        fprintf(stderr, "failed to read message\n");
        abort();
      }
      printf(" [x] Received '%.*s'\n", (int)envelope.message.body.len,
             (char*)envelope.message.body.bytes);
      amqp_destroy_envelope(&envelope);
    }
  }

  /* The above loop goes forever, this is never hit, but for completeness */
  reply = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
  if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
    fprintf(stderr, "failed to close connection: \n");
    abort();
  }

cleanup1:
  amqp_destroy_connection(conn);
  return res;
}
