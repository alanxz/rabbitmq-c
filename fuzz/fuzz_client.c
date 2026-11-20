// Copyright 2007 - 2026, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

// Stateful client-API harness: drives the post-login rabbitmq-c API surface.
//
// Threat model: a malicious or compromised broker controls every byte the
// client reads back after the connection is up, and applications call the
// amqp_basic_* senders, the queue/channel RPC wrappers, amqp_consume_message,
// amqp_basic_get and amqp_publisher_confirm_wait on that connection. The
// existing fuzzers stop at the wire parser (fuzz_handle_input), the login
// handshake (fuzz_server) and amqp_read_message (fuzz_read_message), so
// amqp_api.c, amqp_consume_message and the RPC wait machinery stay dark.
//
// Input layout: 3 control bytes, a length-bounded properties-table region,
// then the broker byte-stream (login handshake first, then scripted RPC
// replies and deliveries). An in-memory socket (same pattern as fuzz_server)
// serves every client read from that stream and never blocks: it returns
// buffered bytes or AMQP_STATUS_CONNECTION_CLOSED at EOF.
//
// Bounds: input capped at 1 MiB, header body_size walker rejects streams
// delivering more than 1 MiB of body (same walker as fuzz_read_message),
// properties region capped at 255 bytes, publish body at 128 bytes, so the
// fuzzer cannot OOM or hang on its own input.

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>

#include "amqp_private.h"
#include "amqp_socket.h"

#define MAX_INPUT_SIZE (1U << 20)
#define MAX_BODY_SIZE (1U << 20)
/* Publish body length is (props_len & 31) * 4, so the buffer must hold at
 * least 31 * 4 = 124 bytes. */
#define MAX_PUBLISH_BODY 128

struct fuzz_socket_t {
  const struct amqp_socket_class_t *klass;
  const uint8_t *data;
  size_t size;
  size_t offset;
};

static ssize_t fuzz_socket_send(AMQP_UNUSED void *base,
                                AMQP_UNUSED const void *buf, size_t len,
                                AMQP_UNUSED int flags) {
  return (ssize_t)len;
}

static ssize_t fuzz_socket_recv(void *base, void *buf, size_t len,
                                AMQP_UNUSED int flags) {
  struct fuzz_socket_t *socket = (struct fuzz_socket_t *)base;
  size_t remaining = socket->size - socket->offset;
  size_t count;

  if (remaining == 0) {
    return AMQP_STATUS_CONNECTION_CLOSED;
  }

  count = remaining < len ? remaining : len;
  memcpy(buf, socket->data + socket->offset, count);
  socket->offset += count;
  return (ssize_t)count;
}

static int fuzz_socket_open(AMQP_UNUSED void *base, AMQP_UNUSED const char *host,
                            AMQP_UNUSED int port,
                            AMQP_UNUSED const struct timeval *timeout) {
  return AMQP_STATUS_OK;
}

static int fuzz_socket_close(AMQP_UNUSED void *base,
                             AMQP_UNUSED amqp_socket_close_enum force) {
  return AMQP_STATUS_OK;
}

static int fuzz_socket_get_sockfd(AMQP_UNUSED void *base) {
  return 0; /* must be >= 0; -1 is treated as a closed connection */
}

static void fuzz_socket_delete(void *base) { free(base); }

static const struct amqp_socket_class_t fuzz_socket_class = {
    fuzz_socket_send, fuzz_socket_recv,       fuzz_socket_open,
    fuzz_socket_close, fuzz_socket_get_sockfd, fuzz_socket_delete};

static uint64_t read_be64(const uint8_t *data) {
  uint64_t value = 0;
  size_t i;

  for (i = 0; i < 8; i++) {
    value = (value << 8) | data[i];
  }
  return value;
}

/* Same body-size bound walker as fuzz_read_message: reject streams whose
 * header frames promise more than MAX_BODY_SIZE of message body. */
static int body_sizes_are_bounded(const uint8_t *data, size_t size) {
  size_t offset = 0;

  if (size >= 8 && memcmp(data, "AMQP", 4) == 0) {
    offset = 8;
  }

  while (size - offset >= 8) {
    uint32_t payload_size = ((uint32_t)data[offset + 3] << 24) |
                            ((uint32_t)data[offset + 4] << 16) |
                            ((uint32_t)data[offset + 5] << 8) |
                            (uint32_t)data[offset + 6];
    size_t frame_size = (size_t)payload_size + 8;

    if (frame_size > size - offset) {
      break;
    }
    if (data[offset] == AMQP_FRAME_HEADER && payload_size >= 12 &&
        read_be64(data + offset + 11) > MAX_BODY_SIZE) {
      return 0;
    }
    offset += frame_size;
  }

  return 1;
}

extern int LLVMFuzzerTestOneInput(const char *data, size_t size) {
  const uint8_t *bytes = (const uint8_t *)data;
  uint8_t ops, misc, props_len;
  amqp_connection_state_t conn;
  struct fuzz_socket_t *socket;
  const uint8_t *stream;
  size_t stream_size;

  if (size < 4 || size > MAX_INPUT_SIZE) {
    return 0;
  }
  ops = bytes[0];
  misc = bytes[1];
  props_len = bytes[2];
  if ((size_t)props_len + 4 > size) {
    return 0;
  }
  if (!body_sizes_are_bounded(bytes + 3 + (size_t)props_len,
                              size - 3 - (size_t)props_len)) {
    return 0;
  }

  conn = amqp_new_connection();
  if (conn == NULL) {
    return 0;
  }

  socket = (struct fuzz_socket_t *)calloc(1, sizeof(*socket));
  if (socket == NULL) {
    amqp_destroy_connection(conn);
    return 0;
  }
  socket->klass = &fuzz_socket_class;
  stream = bytes + 3 + (size_t)props_len;
  stream_size = size - 3 - (size_t)props_len;
  socket->data = stream;
  socket->size = stream_size;

  /* Takes ownership; amqp_destroy_connection frees it via fuzz_socket_delete. */
  amqp_set_socket(conn, (amqp_socket_t *)socket);

  (void)amqp_login(conn, "/", 0, AMQP_DEFAULT_FRAME_SIZE, 0,
                   AMQP_SASL_METHOD_PLAIN, "guest", "guest");

  if (misc & 0x01) {
    const struct timeval invalid = {-1, 0};
    (void)amqp_set_handshake_timeout(conn, &invalid);
  }
  if (misc & 0x02) {
    const struct timeval rpc_timeout = {0, 1000};
    (void)amqp_set_rpc_timeout(conn, &rpc_timeout);
  }
  if (misc & 0x04) {
    (void)amqp_set_handshake_timeout(conn, NULL);
  }

  if (ops & 0x01) {
    (void)amqp_channel_open(conn, 1);
  }
  if (ops & 0x02) {
    (void)amqp_queue_declare(conn, 1, amqp_cstring_bytes("q"), 0, 0, 0, 0,
                             amqp_empty_table);
  }
  if (ops & 0x04) {
    (void)amqp_queue_bind(conn, 1, amqp_cstring_bytes("q"),
                          amqp_cstring_bytes("e"), amqp_cstring_bytes("k"),
                          amqp_empty_table);
  }
  if (ops & 0x08) {
    (void)amqp_basic_consume(conn, 1, amqp_cstring_bytes("q"),
                             amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
  }
  if (ops & 0x10) {
    amqp_rpc_reply_t get_reply =
        amqp_basic_get(conn, 1, amqp_cstring_bytes("q"), 1);
    (void)get_reply;
  }
  if (ops & 0x20) {
    amqp_pool_t props_pool;
    amqp_basic_properties_t props;
    amqp_bytes_t body;
    uint8_t body_buf[MAX_PUBLISH_BODY];

    memset(&props, 0, sizeof(props));
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");

    init_amqp_pool(&props_pool, 4096);
    if (props_len >= 4) {
      amqp_table_t decoded;
      amqp_bytes_t encoded;
      size_t offset = 0;

      encoded.len = props_len;
      encoded.bytes = (void *)(bytes + 3);
      memset(&decoded, 0, sizeof(decoded));
      if (amqp_decode_table(encoded, &props_pool, &decoded, &offset) ==
          AMQP_STATUS_OK) {
        props._flags |= AMQP_BASIC_HEADERS_FLAG;
        props.headers = decoded;
      }
    }

    memset(body_buf, 0x41, sizeof(body_buf));
    body.len = (size_t)(props_len & 31) * 4;
    body.bytes = body_buf;

    (void)amqp_basic_publish(conn, 1, amqp_cstring_bytes("e"),
                             amqp_cstring_bytes("k"), (misc & 0x40) != 0,
                             (misc & 0x80) != 0, &props, body);
    empty_amqp_pool(&props_pool);
  }
  if (ops & 0x40) {
    (void)amqp_basic_ack(conn, 1, 1, 0);
    (void)amqp_basic_nack(conn, 1, 2, 1, 0);
    (void)amqp_basic_reject(conn, 1, 3, 0);
  }
  if (ops & 0x80) {
    amqp_envelope_t envelope;
    amqp_rpc_reply_t consume_reply;

    memset(&envelope, 0, sizeof(envelope));
    consume_reply = amqp_consume_message(conn, &envelope, NULL, 0);
    if (consume_reply.reply_type == AMQP_RESPONSE_NORMAL) {
      amqp_destroy_envelope(&envelope);
    }

    {
      amqp_publisher_confirm_t confirm;
      amqp_rpc_reply_t confirm_reply;

      memset(&confirm, 0, sizeof(confirm));
      confirm_reply = amqp_publisher_confirm_wait(conn, NULL, &confirm);
      (void)confirm_reply;
    }
  }

  if (misc & 0x08) {
    amqp_maybe_release_buffers(conn);
    amqp_maybe_release_buffers_on_channel(conn, 1);
  }
  if (misc & 0x10) {
    (void)amqp_channel_close(conn, 1, 200);
  }
  if (misc & 0x20) {
    (void)amqp_connection_close(conn, 200);
  }

  amqp_destroy_connection(conn);
  return 0;
}
