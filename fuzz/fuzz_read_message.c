// Copyright 2007 - 2026, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>

#include "amqp_socket.h"

#define MAX_INPUT_SIZE (1U << 20)
#define MAX_BODY_SIZE (1U << 20)

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

static int fuzz_socket_open(AMQP_UNUSED void *base,
                            AMQP_UNUSED const char *host,
                            AMQP_UNUSED int port,
                            AMQP_UNUSED const struct timeval *timeout) {
  return AMQP_STATUS_OK;
}

static int fuzz_socket_close(AMQP_UNUSED void *base,
                             AMQP_UNUSED amqp_socket_close_enum force) {
  return AMQP_STATUS_OK;
}

static int fuzz_socket_get_sockfd(AMQP_UNUSED void *base) { return 0; }

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
  amqp_connection_state_t connection;
  amqp_message_t message;
  struct fuzz_socket_t *socket;
  amqp_rpc_reply_t reply;

  if (size == 0 || size > MAX_INPUT_SIZE ||
      !body_sizes_are_bounded((const uint8_t *)data, size)) {
    return 0;
  }

  connection = amqp_new_connection();
  if (connection == NULL) {
    return 0;
  }

  socket = (struct fuzz_socket_t *)calloc(1, sizeof(*socket));
  if (socket == NULL) {
    amqp_destroy_connection(connection);
    return 0;
  }

  socket->klass = &fuzz_socket_class;
  socket->data = (const uint8_t *)data;
  socket->size = size;
  amqp_set_socket(connection, (amqp_socket_t *)socket);

  reply = amqp_read_message(connection, 1, &message, 0);
  if (reply.reply_type == AMQP_RESPONSE_NORMAL) {
    amqp_destroy_message(&message);
  }

  amqp_destroy_connection(connection);
  return 0;
}