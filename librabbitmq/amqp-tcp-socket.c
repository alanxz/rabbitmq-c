/*
 * Copyright 2012 Michael Steinert
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
#include "amqp-tcp-socket.h"
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
	struct addrinfo *address_list;
	struct addrinfo *addr;
	struct addrinfo hint;
	char port_string[33];
	int status, one = 1; /* for setsockopt */
	status = amqp_socket_init();
	if (status) {
		return status;
	}
	memset(&hint, 0, sizeof hint);
	hint.ai_family = PF_UNSPEC; /* PF_INET or PF_INET6 */
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;
	(void)snprintf(port_string, sizeof port_string, "%d", port);
	port_string[sizeof port_string - 1] = '\0';
	status = getaddrinfo(host, port_string, &hint, &address_list);
	if (status) {
		return -ERROR_GETHOSTBYNAME_FAILED;
	}
	for (addr = address_list; addr; addr = addr->ai_next) {
		/*
		 * This cast is to squash warnings on Win64, see:
		 * http://bit.ly/PTxfCU
		 */
		self->sockfd = (int)amqp_socket_socket(addr->ai_family,
						       addr->ai_socktype,
						       addr->ai_protocol);
		if (-1 == self->sockfd) {
			status = -amqp_os_socket_error();
			continue;
		}
#ifdef DISABLE_SIGPIPE_WITH_SETSOCKOPT
		status = amqp_socket_setsockopt(self->sockfd, SOL_SOCKET,
						SO_NOSIGPIPE, &one,
						sizeof one);
		if (0 > status) {
			status = -amqp_os_socket_error();
			amqp_os_socket_close(self->sockfd);
			continue;
		}
#endif /* DISABLE_SIGPIPE_WITH_SETSOCKOPT */
		status = amqp_socket_setsockopt(self->sockfd, IPPROTO_TCP,
						TCP_NODELAY, &one,
						sizeof one);
		if (0 > status) {
			status = -amqp_os_socket_error();
			amqp_os_socket_close(self->sockfd);
			continue;
		}
		status = connect(self->sockfd, addr->ai_addr, addr->ai_addrlen);
		if (0 > status) {
			status = -amqp_os_socket_error();
			amqp_os_socket_close(self->sockfd);
			continue;
		}
		status = 0;
		break;
	}
	freeaddrinfo(address_list);
	return status;
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
	return status;
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
