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

#include "amqp-ssl.h"
#include "amqp_private.h"
#include <cyassl/ssl.h>
#include <stdlib.h>

struct amqp_ssl_socket_context {
	CYASSL_CTX *ctx;
	CYASSL *ssl;
	char *buffer;
	size_t length;
};

static ssize_t
amqp_ssl_socket_send(AMQP_UNUSED int sockfd,
		     const void *buf,
		     size_t len,
		     AMQP_UNUSED int flags,
		     void *user_data)
{
	struct amqp_ssl_socket_context *self = user_data;
	return CyaSSL_write(self->ssl, buf, len);
}

static ssize_t
amqp_ssl_socket_writev(AMQP_UNUSED int sockfd,
		       const struct iovec *iov,
		       int iovcnt,
		       void *user_data)
{
	struct amqp_ssl_socket_context *self = user_data;
	ssize_t written = -1;
	char *bufferp;
	size_t bytes;
	int i;
	bytes = 0;
	for (i = 0; i < iovcnt; ++i) {
		bytes += iov[i].iov_len;
	}
	if (self->length < bytes) {
		free(self->buffer);
		self->buffer = malloc(bytes);
		if (!self->buffer) {
			self->length = 0;
			goto exit;
		}
		self->length = bytes;
	}
	bufferp = self->buffer;
	for (i = 0; i < iovcnt; ++i) {
		memcpy(bufferp, iov[i].iov_base, iov[i].iov_len);
		bufferp += iov[i].iov_len;
	}
	written = CyaSSL_write(self->ssl, self->buffer, bytes);
exit:
	return written;
}

static ssize_t
amqp_ssl_socket_recv(AMQP_UNUSED int sockfd,
		     void *buf,
		     size_t len,
		     AMQP_UNUSED int flags,
		     void *user_data)
{
	struct amqp_ssl_socket_context *self = user_data;
	return CyaSSL_read(self->ssl, buf, len);
}

static int
amqp_ssl_socket_close(int sockfd,
		      void *user_data)
{
	int status = -1;
	struct amqp_ssl_socket_context *self = user_data;
	if (self) {
		CyaSSL_free(self->ssl);
		CyaSSL_CTX_free(self->ctx);
		free(self->buffer);
		free(self);
	}
	if (sockfd >= 0) {
		status = amqp_socket_close(sockfd, 0);
	}
	return status;
}

static int
amqp_ssl_socket_error(AMQP_UNUSED void *user_data)
{
	return -1;
}

int
amqp_open_ssl_socket(amqp_connection_state_t state,
		     const char *host,
		     int port,
		     const char *cacert,
		     const char *key,
		     const char *cert)
{
	int sockfd = -1, status;
	struct amqp_ssl_socket_context *self;
	CyaSSL_Init();
	self = calloc(1, sizeof(*self));
	if (!self) {
		goto error;
	}
	self->ctx = CyaSSL_CTX_new(CyaSSLv23_client_method());
	if (!self->ctx) {
		goto error;
	}
	status = CyaSSL_CTX_load_verify_locations(self->ctx, cacert, NULL);
	if (SSL_SUCCESS != status) {
		goto error;
	}
	if (key && cert) {
		status = CyaSSL_CTX_use_PrivateKey_file(self->ctx, key,
							SSL_FILETYPE_PEM);
		if (SSL_SUCCESS != status) {
			goto error;
		}
		status = CyaSSL_CTX_use_certificate_chain_file(self->ctx, cert);
	}
	self->ssl = CyaSSL_new(self->ctx);
	if (!self->ssl) {
		goto error;
	}
	sockfd = amqp_open_socket(host, port);
	if (0 > sockfd) {
		goto error;
	}
	CyaSSL_set_fd(self->ssl, sockfd);
	status = CyaSSL_connect(self->ssl);
	if (SSL_SUCCESS != status) {
		goto error;
	}
	amqp_set_sockfd_full(state, sockfd,
			     amqp_ssl_socket_writev,
			     amqp_ssl_socket_send,
			     amqp_ssl_socket_recv,
			     amqp_ssl_socket_close,
			     amqp_ssl_socket_error,
			     self);
	return sockfd;
error:
	amqp_ssl_socket_close(sockfd, self);
	return -1;
}

void
amqp_set_initialize_ssl_library(amqp_boolean_t do_initialize)
{
}
