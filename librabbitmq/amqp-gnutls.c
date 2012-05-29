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
#include "amqp-ssl.h"
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <stdlib.h>

struct amqp_ssl_socket_context {
	gnutls_session_t session;
	gnutls_certificate_credentials_t credentials;
	char *host;
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
	return gnutls_record_send(self->session, buf, len);
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
		self->length = 0;
	}
	bufferp = self->buffer;
	for (i = 0; i < iovcnt; ++i) {
		memcpy(bufferp, iov[i].iov_base, iov[i].iov_len);
		bufferp += iov[i].iov_len;
	}
	written = gnutls_record_send(self->session, self->buffer, bytes);
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
	return gnutls_record_recv(self->session, buf, len);
}

static int
amqp_ssl_socket_close(int sockfd,
		      void *user_data)
{
	int status = -1;
	struct amqp_ssl_socket_context *self = user_data;
	if (sockfd >= 0) {
		status = amqp_socket_close(sockfd, 0);
	}
	if (self) {
		gnutls_deinit(self->session);
		gnutls_certificate_free_credentials(self->credentials);
		free(self->host);
		free(self->buffer);
		free(self);
	}
	return status;
}

static int
amqp_ssl_socket_error(AMQP_UNUSED void *user_data)
{
	return -1;
}

static int
amqp_ssl_verify(gnutls_session_t session)
{
	int ret;
	unsigned int status, size;
	const gnutls_datum_t *list;
	gnutls_x509_crt_t cert = NULL;
	struct amqp_ssl_socket_context *self = gnutls_session_get_ptr(session);
	ret = gnutls_certificate_verify_peers2(session, &status);
	if (0 > ret) {
		goto error;
	}
	if (status & GNUTLS_CERT_INVALID) {
		goto error;
	}
	if (status & GNUTLS_CERT_SIGNER_NOT_FOUND) {
		goto error;
	}
	if (status & GNUTLS_CERT_REVOKED) {
		goto error;
	}
	if (status & GNUTLS_CERT_EXPIRED) {
		goto error;
	}
	if (status & GNUTLS_CERT_NOT_ACTIVATED) {
		goto error;
	}
	if (gnutls_certificate_type_get(session) != GNUTLS_CRT_X509) {
		goto error;
	}
	if (gnutls_x509_crt_init(&cert) < 0) {
		goto error;
	}
	list = gnutls_certificate_get_peers(session, &size);
	if (!list) {
		goto error;
	}
	ret = gnutls_x509_crt_import(cert, &list[0], GNUTLS_X509_FMT_DER);
	if (0 > ret) {
		goto error;
	}
	if (!gnutls_x509_crt_check_hostname(cert, self->host)) {
		goto error;
	}
	gnutls_x509_crt_deinit(cert);
	return 0;
error:
	if (cert) {
		gnutls_x509_crt_deinit (cert);
	}
	return GNUTLS_E_CERTIFICATE_ERROR;
}

int
amqp_open_ssl_socket(amqp_connection_state_t state,
		     const char *host,
		     int port,
		     const char *cacert,
		     const char *key,
		     const char *cert)
{
	struct amqp_ssl_socket_context *self;
	const char *error;
	int sockfd = -1;
	int ret;
	gnutls_global_init();
	self = calloc(1, sizeof(*self));
	if (!self) {
		goto error;
	}
	self->host = strdup(host);
	if (!self->host) {
		goto error;
	}
	ret = gnutls_certificate_allocate_credentials(&self->credentials);
	if (GNUTLS_E_SUCCESS != ret) {
		goto error;
	}
	ret = gnutls_certificate_set_x509_trust_file(self->credentials,
							cacert,
							GNUTLS_X509_FMT_PEM);
	if (0 > ret) {
		goto error;
	}
	gnutls_certificate_set_verify_function(self->credentials,
					       amqp_ssl_verify);
	if (key && cert) {
		ret = gnutls_certificate_set_x509_key_file(
				self->credentials, cert, key,
				GNUTLS_X509_FMT_PEM);
		if (0 > ret) {
			goto error;
		}
	}
	ret = gnutls_init(&self->session, GNUTLS_CLIENT);
	if (GNUTLS_E_SUCCESS != ret) {
		goto error;
	}
	gnutls_session_set_ptr(self->session, self);
	ret = gnutls_priority_set_direct(self->session, "NORMAL", &error);
	if (GNUTLS_E_SUCCESS != ret) {
		goto error;
	}
	ret = gnutls_credentials_set(self->session, GNUTLS_CRD_CERTIFICATE,
				     self->credentials);
	if (GNUTLS_E_SUCCESS != ret) {
		goto error;
	}
	sockfd = amqp_open_socket(host, port);
	if (0 > sockfd) {
		goto error;
	}
	gnutls_transport_set_ptr(self->session, (gnutls_transport_ptr_t)sockfd);
	do {
		ret = gnutls_handshake(self->session);
	} while (ret < 0 && !gnutls_error_is_fatal(ret));
	amqp_set_sockfd_full(state, sockfd,
			     amqp_ssl_socket_writev,
			     amqp_ssl_socket_send,
			     amqp_ssl_socket_recv,
			     amqp_ssl_socket_close,
			     amqp_ssl_socket_error,
			     self);
exit:
	return sockfd;
error:
	amqp_ssl_socket_close(sockfd, self);
	sockfd = -1;
	goto exit;
}

void
amqp_set_initialize_ssl_library(amqp_boolean_t do_initialize)
{
}
