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
#include <polarssl/ctr_drbg.h>
#include <polarssl/entropy.h>
#include <polarssl/net.h>
#include <polarssl/ssl.h>
#include <stdlib.h>

struct amqp_ssl_socket_context {
	int sockfd;
	entropy_context *entropy;
	ctr_drbg_context *ctr_drbg;
	x509_cert *cacert;
	rsa_context *key;
	x509_cert *cert;
	ssl_context *ssl;
	ssl_session *session;
};

static ssize_t
amqp_ssl_socket_send(AMQP_UNUSED int sockfd,
		     const void *buf,
		     size_t len,
		     AMQP_UNUSED int flags,
		     void *user_data)
{
	struct amqp_ssl_socket_context *self = user_data;
	return ssl_write(self->ssl, buf, len);
}

static ssize_t
amqp_ssl_socket_writev(AMQP_UNUSED int sockfd,
		       const struct iovec *iov,
		       int iovcnt,
		       void *user_data)
{
	struct amqp_ssl_socket_context *self = user_data;
	char *buffer, *bufferp;
	ssize_t written = -1;
	size_t bytes;
	int i;
	bytes = 0;
	for (i = 0; i < iovcnt; ++i) {
		bytes += iov[i].iov_len;
	}
	buffer = malloc(bytes);
	if (!buffer) {
		goto exit;
	}
	bufferp = buffer;
	for (i = 0; i < iovcnt; ++i) {
		memcpy(bufferp, iov[i].iov_base, iov[i].iov_len);
		bufferp += iov[i].iov_len;
	}
	written = ssl_write(self->ssl, (const unsigned char *)buffer, bytes);
exit:
	free(buffer);
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
	return ssl_read(self->ssl, buf, len);
}

static int
amqp_ssl_socket_close(int sockfd,
		      void *user_data)
{
	int status = -1;
	struct amqp_ssl_socket_context *self = user_data;
	if (self) {
		free(self->entropy);
		free(self->ctr_drbg);
		x509_free(self->cacert);
		free(self->cacert);
		rsa_free(self->key);
		free(self->key);
		x509_free(self->cert);
		free(self->cert);
		ssl_free(self->ssl);
		free(self->ssl);
		free(self->session);
		free(self);
		if (self->sockfd >= 0) {
			net_close(sockfd);
			status = 0;
		}
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
	int status;
	struct amqp_ssl_socket_context *self;
	self = calloc(1, sizeof(*self));
	if (!self) {
		goto error;
	}
	self->entropy = calloc(1, sizeof(*self->entropy));
	if (!self->entropy) {
		goto error;
	}
	self->sockfd = -1;
	entropy_init(self->entropy);
	self->ctr_drbg = calloc(1, sizeof(*self->ctr_drbg));
	if (!self->ctr_drbg) {
		goto error;
	}
	status = ctr_drbg_init(self->ctr_drbg, entropy_func, self->entropy,
			       NULL, 0);
	if (status) {
		goto error;
	}
	self->cacert = calloc(1, sizeof(*self->cacert));
	if (!self->cacert) {
		goto error;
	}
	status = x509parse_crtfile(self->cacert, cacert);
	if (status) {
		goto error;
	}
	if (key && cert) {
		self->key = calloc(1, sizeof(*self->key));
		if (!self->key) {
			goto error;
		}
		status = x509parse_keyfile(self->key, key, NULL);
		if (status) {
			goto error;
		}
		self->cert = calloc(1, sizeof(*self->cert));
		if (!self->cert) {
			goto error;
		}
		status = x509parse_crtfile(self->cert, cert);
		if (status) {
			goto error;
		}
	}
	status = net_connect(&self->sockfd, host, port);
	if (status) {
		goto error;
	}
	self->ssl = calloc(1, sizeof(*self->ssl));
	if (!self->ssl) {
		goto error;
	}
	status = ssl_init(self->ssl);
	if (status) {
		goto error;
	}
	ssl_set_endpoint(self->ssl, SSL_IS_CLIENT);
	ssl_set_authmode(self->ssl, SSL_VERIFY_REQUIRED);
	ssl_set_ca_chain(self->ssl, self->cacert, NULL, host);
	ssl_set_rng(self->ssl, ctr_drbg_random, self->ctr_drbg);
	ssl_set_bio(self->ssl, net_recv, &self->sockfd,
		    net_send, &self->sockfd);
	ssl_set_ciphersuites(self->ssl, ssl_default_ciphersuites);
	self->session = calloc(1, sizeof(*self->session));
	if (!self->session) {
		goto error;
	}
	ssl_set_session(self->ssl, 0, 0, self->session);
	if (self->key && self->cert) {
		ssl_set_own_cert(self->ssl, self->cert, self->key);
	}
	while (0 != (status = ssl_handshake(self->ssl))) {
		switch (status) {
		case POLARSSL_ERR_NET_WANT_READ:
		case POLARSSL_ERR_NET_WANT_WRITE:
			continue;
		default:
			goto error;
		}
	}
	amqp_set_sockfd_full(state, self->sockfd,
			     amqp_ssl_socket_writev,
			     amqp_ssl_socket_send,
			     amqp_ssl_socket_recv,
			     amqp_ssl_socket_close,
			     amqp_ssl_socket_error,
			     self);
	return self->sockfd;
error:
	amqp_ssl_socket_close(self->sockfd, self);
	return -1;
}
