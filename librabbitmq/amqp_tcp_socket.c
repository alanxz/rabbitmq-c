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

#include "amqp_private.h"
#include "amqp_tcp_socket.h"
#include <stdio.h>
#include <stdlib.h>

struct amqp_tcp_socket_t {
  const struct amqp_socket_class_t *klass;
  int sockfd;
};

static ssize_t
amqp_tcp_socket_writev(void *base, const struct iovec *iov, int iovcnt)
{
  struct amqp_tcp_socket_t *self = (struct amqp_tcp_socket_t *)base;
  return amqp_os_socket_writev(self->sockfd, iov, iovcnt);
}

static ssize_t
amqp_tcp_socket_send(void *base, const void *buf, size_t len, int flags)
{
  struct amqp_tcp_socket_t *self = (struct amqp_tcp_socket_t *)base;
  return send(self->sockfd, buf, len, flags);
}

static ssize_t
amqp_tcp_socket_recv(void *base, void *buf, size_t len, int flags)
{
  struct amqp_tcp_socket_t *self = (struct amqp_tcp_socket_t *)base;
  return recv(self->sockfd, buf, len, flags);
}

static int
amqp_tcp_socket_open(void *base, const char *host, int port)
{
  struct amqp_tcp_socket_t *self = (struct amqp_tcp_socket_t *)base;
  self->sockfd = amqp_open_socket(host, port);
  if (0 > self->sockfd) {
    return -1;
  }
  return 0;
}

static int
amqp_tcp_socket_close(void *base)
{
  struct amqp_tcp_socket_t *self = (struct amqp_tcp_socket_t *)base;
  int status = -1;
  if (self) {
    status = amqp_os_socket_close(self->sockfd);
    free(self);
  }

  if (0 == status) {
    return AMQP_STATUS_OK;
  } else {
    return AMQP_STATUS_SOCKET_ERROR;
  }
}

static int
amqp_tcp_socket_error(AMQP_UNUSED void *base)
{
  return amqp_os_socket_error();
}

static int
amqp_tcp_socket_get_sockfd(void *base)
{
  struct amqp_tcp_socket_t *self = (struct amqp_tcp_socket_t *)base;
  return self->sockfd;
}

static const struct amqp_socket_class_t amqp_tcp_socket_class = {
  amqp_tcp_socket_writev, /* writev */
  amqp_tcp_socket_send, /* send */
  amqp_tcp_socket_recv, /* recv */
  amqp_tcp_socket_open, /* open */
  amqp_tcp_socket_close, /* close */
  amqp_tcp_socket_error, /* error */
  amqp_tcp_socket_get_sockfd /* get_sockfd */
};

amqp_socket_t *
amqp_tcp_socket_new(void)
{
  struct amqp_tcp_socket_t *self = calloc(1, sizeof(*self));
  if (!self) {
    return NULL;
  }
  self->klass = &amqp_tcp_socket_class;
  self->sockfd = -1;
  return (amqp_socket_t *)self;
}

void
amqp_tcp_socket_set_sockfd(amqp_socket_t *base, int sockfd)
{
  struct amqp_tcp_socket_t *self;
  if (base->klass != &amqp_tcp_socket_class) {
    amqp_abort("<%p> is not of type amqp_tcp_socket_t", base);
  }
  self = (struct amqp_tcp_socket_t *)base;
  self->sockfd = sockfd;
}
