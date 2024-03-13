// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include "amqp_openssl_bio.h"
#include "amqp_socket.h"

#include <assert.h>
#include <errno.h>
#if ((defined(_WIN32)) || (defined(__MINGW32__)) || (defined(__MINGW64__)))
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#endif

#ifdef MSG_NOSIGNAL
#define AMQP_USE_AMQP_BIO
#endif

static int amqp_ssl_bio_initialized = 0;

#ifdef AMQP_USE_AMQP_BIO

static BIO_METHOD *amqp_bio_method;

static int amqp_openssl_bio_should_retry(int res) {
  if (res == -1) {
    int err = amqp_os_socket_error();
    if (
#ifdef EWOULDBLOCK
        err == EWOULDBLOCK ||
#endif
#ifdef WSAEWOULDBLOCK
        err == WSAEWOULDBLOCK ||
#endif
#ifdef ENOTCONN
        err == ENOTCONN ||
#endif
#ifdef EINTR
        err == EINTR ||
#endif
#ifdef EAGAIN
        err == EAGAIN ||
#endif
#ifdef EPROTO
        err == EPROTO ||
#endif
#ifdef EINPROGRESS
        err == EINPROGRESS ||
#endif
#ifdef EALREADY
        err == EALREADY ||
#endif
        0) {
      return 1;
    }
  }
  return 0;
}

static int amqp_openssl_bio_write(BIO *b, const char *in, int inl) {
  int flags = 0;
  int fd;
  int res;

#ifdef MSG_NOSIGNAL
  flags |= MSG_NOSIGNAL;
#endif

  BIO_get_fd(b, &fd);
  res = send(fd, in, inl, flags);

  BIO_clear_retry_flags(b);
  if (res <= 0 && amqp_openssl_bio_should_retry(res)) {
    BIO_set_retry_write(b);
  }

  return res;
}

static int amqp_openssl_bio_read(BIO *b, char *out, int outl) {
  int flags = 0;
  int fd;
  int res;

#ifdef MSG_NOSIGNAL
  flags |= MSG_NOSIGNAL;
#endif

  BIO_get_fd(b, &fd);
  res = recv(fd, out, outl, flags);

  BIO_clear_retry_flags(b);
  if (res <= 0 && amqp_openssl_bio_should_retry(res)) {
    BIO_set_retry_read(b);
  }

  return res;
}
#endif /* AMQP_USE_AMQP_BIO */

int amqp_openssl_bio_init(void) {
  assert(!amqp_ssl_bio_initialized);
#ifdef AMQP_USE_AMQP_BIO
  if (!(amqp_bio_method = BIO_meth_new(BIO_TYPE_SOCKET, "amqp_bio_method"))) {
    return AMQP_STATUS_NO_MEMORY;
  }
#ifdef OPENSSL_IS_BORINGSSL
  BIO_meth_set_create(amqp_bio_method, BIO_s_socket()->create);
  BIO_meth_set_destroy(amqp_bio_method, BIO_s_socket()->destroy);
  BIO_meth_set_ctrl(amqp_bio_method, BIO_s_socket()->ctrl);
  BIO_meth_set_read(amqp_bio_method, BIO_s_socket()->bread);
  BIO_meth_set_write(amqp_bio_method, BIO_s_socket()->bwrite);
  BIO_meth_set_gets(amqp_bio_method, BIO_s_socket()->bgets);
  BIO_meth_set_puts(amqp_bio_method, BIO_s_socket()->bputs);
#else
  BIO_meth_set_create(amqp_bio_method, BIO_meth_get_create(BIO_s_socket()));
  BIO_meth_set_destroy(amqp_bio_method, BIO_meth_get_destroy(BIO_s_socket()));
  BIO_meth_set_ctrl(amqp_bio_method, BIO_meth_get_ctrl(BIO_s_socket()));
  BIO_meth_set_callback_ctrl(amqp_bio_method,
                             BIO_meth_get_callback_ctrl(BIO_s_socket()));
  BIO_meth_set_read(amqp_bio_method, BIO_meth_get_read(BIO_s_socket()));
  BIO_meth_set_write(amqp_bio_method, BIO_meth_get_write(BIO_s_socket()));
  BIO_meth_set_gets(amqp_bio_method, BIO_meth_get_gets(BIO_s_socket()));
  BIO_meth_set_puts(amqp_bio_method, BIO_meth_get_puts(BIO_s_socket()));
#endif

  BIO_meth_set_write(amqp_bio_method, amqp_openssl_bio_write);
  BIO_meth_set_read(amqp_bio_method, amqp_openssl_bio_read);
#endif

  amqp_ssl_bio_initialized = 1;
  return AMQP_STATUS_OK;
}

void amqp_openssl_bio_destroy(void) {
  assert(amqp_ssl_bio_initialized);
#ifdef AMQP_USE_AMQP_BIO
  BIO_meth_free(amqp_bio_method);
  amqp_bio_method = NULL;
#endif
  amqp_ssl_bio_initialized = 0;
}

BIO_METHOD_PTR amqp_openssl_bio(void) {
  assert(amqp_ssl_bio_initialized);
#ifdef AMQP_USE_AMQP_BIO
  return amqp_bio_method;
#else
  return BIO_s_socket();
#endif
}
