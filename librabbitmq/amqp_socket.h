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

/**
 * An abstract socket interface.
 */

#ifndef AMQP_SOCKET_H
#define AMQP_SOCKET_H

#include "amqp.h"
#include "socket.h"

AMQP_BEGIN_DECLS

/* Socket callbacks. */
typedef ssize_t (*amqp_socket_writev_fn)(void *, const struct iovec *, int);
typedef ssize_t (*amqp_socket_send_fn)(void *, const void *, size_t, int);
typedef ssize_t (*amqp_socket_recv_fn)(void *, void *, size_t, int);
typedef int (*amqp_socket_open_fn)(void *, const char *, int);
typedef int (*amqp_socket_close_fn)(void *);
typedef int (*amqp_socket_error_fn)(void *);
typedef int (*amqp_socket_get_sockfd_fn)(void *);

/** V-table for amqp_socket_t */
struct amqp_socket_class_t {
  amqp_socket_writev_fn writev;
  amqp_socket_send_fn send;
  amqp_socket_recv_fn recv;
  amqp_socket_open_fn open;
  amqp_socket_close_fn close;
  amqp_socket_error_fn error;
  amqp_socket_get_sockfd_fn get_sockfd;
};

/** Abstract base class for amqp_socket_t */
struct amqp_socket_t_ {
  const struct amqp_socket_class_t *klass;
};

/**
 * Write to a socket.
 *
 * This function is analagous to writev(2).
 *
 * \param [in,out] self A socket object.
 * \param [in] iov One or more data vecors.
 * \param [in] iovcnt The number of vectors in \e iov.
 *
 * \return The number of bytes written, or -1 if an error occurred.
 */
ssize_t
amqp_socket_writev(amqp_socket_t *self, const struct iovec *iov, int iovcnt);

/**
 * Send a message from a socket.
 *
 * This function is analagous to send(2).
 *
 * \param [in,out] self A socket object.
 * \param [in] buf A buffer to read from.
 * \param [in] len The number of bytes in \e buf.
 * \param [in] flags Send flags, implementation specific.
 *
 * \return The number of bytes sent, or -1 if an error occurred.
 */
ssize_t
amqp_socket_send(amqp_socket_t *self, const void *buf, size_t len, int flags);

/**
 * Receive a message from a socket.
 *
 * This function is analagous to recv(2).
 *
 * \param [in,out] self A socket object.
 * \param [out] buf A buffer to write to.
 * \param [in] len The number of bytes at \e buf.
 * \param [in] flags Receive flags, implementation specific.
 *
 * \return The number of bytes received, or -1 if an error occurred.
 */
ssize_t
amqp_socket_recv(amqp_socket_t *self, void *buf, size_t len, int flags);

AMQP_END_DECLS

#endif /* AMQP_SOCKET_H */
