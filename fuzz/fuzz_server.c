// Copyright 2007 - 2022, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>

#include "amqp_private.h"
#include "amqp_socket.h"

// In-memory socket: feeds the fuzz buffer to the client as the bytes a broker
// would send, with no real socket or thread, so amqp_login() drives the full
// client handshake (Connection.Start -> Tune -> Open -> OpenOk) from the input.

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
  struct fuzz_socket_t *self = (struct fuzz_socket_t *)base;
  size_t remaining = self->size - self->offset;
  size_t n;

  if (remaining == 0) {
    return AMQP_STATUS_CONNECTION_CLOSED;
  }
  n = remaining < len ? remaining : len;
  memcpy(buf, self->data + self->offset, n);
  self->offset += n;
  return (ssize_t)n;
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

extern int LLVMFuzzerTestOneInput(const char *data, size_t size) {
  amqp_connection_state_t conn;
  struct fuzz_socket_t *sock;

  if (size == 0) {
    return 0;
  }

  conn = amqp_new_connection();
  if (conn == NULL) {
    return 0;
  }

  sock = (struct fuzz_socket_t *)calloc(1, sizeof(*sock));
  if (sock == NULL) {
    amqp_destroy_connection(conn);
    return 0;
  }
  sock->klass = &fuzz_socket_class;
  sock->data = (const uint8_t *)data;
  sock->size = size;

  // Takes ownership; amqp_destroy_connection frees it via fuzz_socket_delete.
  amqp_set_socket(conn, (amqp_socket_t *)sock);

  amqp_login(conn, "/", 0, AMQP_DEFAULT_FRAME_SIZE, 0, AMQP_SASL_METHOD_PLAIN,
             "guest", "guest");

  amqp_destroy_connection(conn);
  return 0;
}
