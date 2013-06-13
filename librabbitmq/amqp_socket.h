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

#ifdef _WIN32
# include <WinSock2.h>
#endif

AMQP_BEGIN_DECLS

int
amqp_os_socket_error(void);

int
amqp_os_socket_close(int sockfd);

/* Socket callbacks. */
typedef ssize_t (*amqp_socket_writev_fn)(void *, struct iovec *, int);
typedef ssize_t (*amqp_socket_send_fn)(void *, const void *, size_t);
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


#ifdef _WIN32
/* WinSock2 calls iovec WSABUF with different parameter names.
 * this is really a WSABUF with different names
 */
struct iovec {
  u_long iov_len;
  char FAR *iov_base;
};
#endif

/**
 * Write to a socket.
 *
 * This function wraps writev(2) functionality.
 *
 * This function will only reutrn on error, or when all of the bytes referred
 * to in iov have been sent. NOTE: this function may modify the iov struct.
 *
 * \param [in,out] self A socket object.
 * \param [in] iov One or more data vecors.
 * \param [in] iovcnt The number of vectors in \e iov.
 *
 * \return AMQP_STATUS_OK on success. amqp_status_enum value otherwise
 */
ssize_t
amqp_socket_writev(amqp_socket_t *self, struct iovec *iov, int iovcnt);

/**
 * Send a message from a socket.
 *
 * This function wraps send(2) functionality.
 *
 * This function will only return on error, or when all of the bytes in buf
 * have been sent, or when an error occurs.
 *
 * \param [in,out] self A socket object.
 * \param [in] buf A buffer to read from.
 * \param [in] len The number of bytes in \e buf.
 *
 * \return AMQP_STATUS_OK on success. amqp_status_enum value otherwise
 */
ssize_t
amqp_socket_send(amqp_socket_t *self, const void *buf, size_t len);

/**
 * Receive a message from a socket.
 *
 * This function wraps recv(2) functionality.
 *
 * \param [in,out] self A socket object.
 * \param [out] buf A buffer to write to.
 * \param [in] len The number of bytes at \e buf.
 * \param [in] flags Receive flags, implementation specific.
 *
 * \return The number of bytes received, or < 0 on error (\ref amqp_status_enum)
 */
ssize_t
amqp_socket_recv(amqp_socket_t *self, void *buf, size_t len, int flags);

AMQP_END_DECLS

#endif /* AMQP_SOCKET_H */
