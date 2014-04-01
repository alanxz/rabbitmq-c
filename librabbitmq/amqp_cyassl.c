/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
/*
 * Copyright 2012-2013 Michael Steinert
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef CONFIG_APP
#include  "incidentLog/log.h"
#endif

#include "amqp_ssl_socket.h"
#include "amqp_private.h"
#include "lwip/sockets.h"
#include <cyassl/ssl.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>

#ifndef AMQP_USE_UNTESTED_SSL_BACKEND
# error This SSL backend is alpha quality and likely contains errors.\
  -DAMQP_USE_UNTESTED_SSL_BACKEND to use this backend
#endif

struct amqp_ssl_socket_t {
  const struct amqp_socket_class_t *klass;
  CYASSL_CTX *ctx;
  CYASSL *ssl;
  int sockfd;
  char *buffer;
  size_t length;
  int last_error;
};

CYASSL_CTX *amqp_ssl_socket_get_cyassl_ctx(amqp_socket_t *base)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  return self->ctx;
}

CYASSL *amqp_ssl_socket_get_cyassl_session_object(amqp_socket_t *base)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  return self->ssl;
}

static ssize_t
amqp_ssl_socket_send_inner(void *base, const void *buf, size_t len, int flags)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  ssize_t res;

  const char *buf_left = buf;
  ssize_t len_left = len;

#ifdef MSG_NOSIGNAL
  flags |= MSG_NOSIGNAL;
#endif

start:
  res = CyaSSL_send(self->ssl, buf_left, len_left, flags);

  if (res < 0) {
    self->last_error = CyaSSL_get_error(self->ssl,res);
    if (EINTR == self->last_error) {
      goto start;
    } else {
      res = AMQP_STATUS_SOCKET_ERROR;
    }
  } else {
    if (res == len_left) {
      self->last_error = 0;
      res = AMQP_STATUS_OK;
    } else {
      buf_left += res;
      len_left -= res;
      goto start;
    }
  }

  return res;
}

static ssize_t
amqp_ssl_socket_send(void *base, const void *buf, size_t len)
{
  return amqp_ssl_socket_send_inner(base, buf, len, 0);
}

static ssize_t
amqp_ssl_socket_writev(void *base, struct iovec *iov, int iovcnt)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  ssize_t ret;

  int i;
  for (i = 0; i < iovcnt - 1; ++i) {
    ret = amqp_ssl_socket_send_inner(self, iov[i].iov_base, iov[i].iov_len, MSG_MORE);
    if (ret != AMQP_STATUS_OK) {
      goto exit;
    }
  }
  ret = amqp_ssl_socket_send_inner(self, iov[i].iov_base, iov[i].iov_len, 0);

exit:
  return ret;
}

static ssize_t
amqp_ssl_socket_recv(void *base, void *buf, size_t len, int flags)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  ssize_t ret;

start:
  ret = CyaSSL_recv(self->ssl, buf, len, flags);

  if (0 > ret) {
    self->last_error = CyaSSL_get_error(self->ssl,ret);
    if (EINTR == self->last_error) {
      goto start;
    } else {
      ret = AMQP_STATUS_SOCKET_ERROR;
    }
  } else if (0 == ret) {
    ret = AMQP_STATUS_CONNECTION_CLOSED;
  }

  return ret;
}

static int
amqp_ssl_socket_get_sockfd(void *base)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  return self->sockfd;
}

static int
amqp_ssl_socket_close(void *base)
{
  int status = -1;
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  if (self->sockfd >= 0) {
    status = amqp_os_socket_close(self->sockfd);
  }
  if (self) {
    if (self->ssl) {
      CyaSSL_free(self->ssl);
    }
    if (self->ctx) {
      CyaSSL_CTX_free(self->ctx);
    }
  }
  return status;
}

static void amqp_ssl_socket_delete(void *base)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;

  if (self) {
    amqp_ssl_socket_close(self);
    free(self->buffer);
    free(self);
  }
}

static int
amqp_ssl_socket_error(void *base)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  return self->last_error;
}

#ifndef CONFIG_RABBITMQ_TINY_EMBEDDED_ENA

char *
amqp_ssl_error_string(AMQP_UNUSED int err)
{
  return strdup("A ssl socket error occurred.");
}

#else

char *
amqp_ssl_error_string(AMQP_UNUSED int err)
{
  return "A ssl socket error occurred.";
}

#endif

static int
amqp_ssl_socket_open(void *base, const char *host, int port, struct timeval *timeout)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  self->last_error = AMQP_STATUS_OK;

  self->ssl = CyaSSL_new(self->ctx);
  if (NULL == self->ssl) {
    self->last_error = AMQP_STATUS_SSL_ERROR;
    return self->last_error;
  }

  self->sockfd = amqp_open_socket_noblock(host, port, timeout);
  if (0 > self->sockfd) {
    self->last_error = - self->sockfd;
    return AMQP_STATUS_SOCKET_ERROR;;
  }
  CyaSSL_set_fd(self->ssl, self->sockfd);

  int status = CyaSSL_connect(self->ssl);
  logDebug("%d=CyaSSL_connect",status);
  if (SSL_SUCCESS != status) {
    logOffNominal( "CyaSSL_connect failed = %d", status );
    self->last_error = AMQP_STATUS_SSL_ERROR;
    return self->last_error;
  }
  return AMQP_STATUS_OK;
}

static const struct amqp_socket_class_t amqp_ssl_socket_class = {
  amqp_ssl_socket_writev, /* writev */
  amqp_ssl_socket_send, /* send */
  amqp_ssl_socket_recv, /* recv */
  amqp_ssl_socket_open, /* open */
  amqp_ssl_socket_close, /* close */
  amqp_ssl_socket_get_sockfd, /* get_sockfd */
  amqp_ssl_socket_delete /* delete */
};

amqp_socket_t *
amqp_ssl_socket_new(amqp_connection_state_t state)
{
  struct amqp_ssl_socket_t *self = calloc(1, sizeof(*self));
  assert(self);
  CyaSSL_Init();
  self->ctx = CyaSSL_CTX_new(CyaTLSv1_2_client_method());
  //self->ctx = CyaSSL_CTX_new(CyaSSLv23_client_method());
  assert(self->ctx);
  self->klass = &amqp_ssl_socket_class;
  self->sockfd = -1;

  amqp_set_socket(state, (amqp_socket_t *)self);

  return (amqp_socket_t *)self;
}

#if defined(CONFIG_RABBITMQ_USE_CYASSL_BUFFER) && CONFIG_RABBITMQ_USE_CYASSL_BUFFER
int
amqp_ssl_socket_set_cacert_buffer(amqp_socket_t *base,
                           const char *cacert,
                           size_t certSize,
                           int type)
{
  int status;
  struct amqp_ssl_socket_t *self;
  if (base->klass != &amqp_ssl_socket_class) {
    amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
  }
  self = (struct amqp_ssl_socket_t *)base;
  status = CyaSSL_CTX_load_verify_buffer(self->ctx, (const unsigned char*)cacert, certSize, type);
  if (SSL_SUCCESS != status) {
    return -1;
  }
  return 0;
}
#endif

#if !defined(NO_FILESYSTEM) && !defined(NO_CERTS)
int
amqp_ssl_socket_set_cacert(amqp_socket_t *base,
                           const char *cacert)
{
  int status;
  struct amqp_ssl_socket_t *self;
  if (base->klass != &amqp_ssl_socket_class) {
    amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
  }
  self = (struct amqp_ssl_socket_t *)base;
  status = CyaSSL_CTX_load_verify_locations(self->ctx, cacert, NULL);
  if (SSL_SUCCESS != status) {
    return -1;
  }
  return 0;
}
#endif

#if defined(CONFIG_RABBITMQ_USE_CYASSL_BUFFER) && CONFIG_RABBITMQ_USE_CYASSL_BUFFER
int
amqp_ssl_socket_set_key_buffer(amqp_socket_t *base,
                                   const char *cert,
                                   const size_t certSize,
                                   const char *key,
                                   const size_t keySize,
                                   const int keyType)
{
  int status;
  struct amqp_ssl_socket_t *self;
  if (base->klass != &amqp_ssl_socket_class) {
    amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
  }
  self = (struct amqp_ssl_socket_t *)base;
  status = CyaSSL_CTX_use_PrivateKey_buffer(
               self->ctx,
               (const unsigned char*)key,
               keySize,
               keyType);
  if (SSL_SUCCESS != status) {
    return -1;
  }

  status = CyaSSL_CTX_use_certificate_chain_buffer(self->ctx, (const unsigned char*)cert, certSize);
  if (SSL_SUCCESS != status) {
    return -1;
  }

  return 0;
}
#endif


#if !defined(NO_FILESYSTEM) && !defined(NO_CERTS)
int
amqp_ssl_socket_set_key(amqp_socket_t *base,
                        const char *cert,
                        const char *key)
{
  int status;
  struct amqp_ssl_socket_t *self;
  if (base->klass != &amqp_ssl_socket_class) {
    amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
  }
  self = (struct amqp_ssl_socket_t *)base;
  status = CyaSSL_CTX_use_PrivateKey_file(self->ctx, key,
                                          SSL_FILETYPE_PEM);
  if (SSL_SUCCESS != status) {
    return -1;
  }

  status = CyaSSL_CTX_use_certificate_chain_file(self->ctx, cert);
  if (SSL_SUCCESS != status) {
    return -1;
  }

  return 0;
}
#endif

void
amqp_ssl_socket_set_verify(AMQP_UNUSED amqp_socket_t *base,
                           AMQP_UNUSED amqp_boolean_t verify)
{
  /* noop for CyaSSL */
  logFatal("Not Implemented.",0);

}

void
amqp_set_initialize_ssl_library(AMQP_UNUSED amqp_boolean_t do_initialize)
{
	  logFatal("Not Implemented.",0);
}
