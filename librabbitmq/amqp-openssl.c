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
#include "threads.h"
#include <ctype.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdlib.h>

static int initialize_openssl(void);
static int destroy_openssl(void);

static int open_ssl_connections = 0;
static amqp_boolean_t do_initialize_openssl = 1;
static amqp_boolean_t openssl_initialized = 0;

#ifdef ENABLE_THREAD_SAFETY
static unsigned long amqp_ssl_threadid_callback(void);
static void amqp_ssl_locking_callback(int mode, int n, const char *file, int line);

#ifdef _WIN32
static long win32_create_mutex = 0;
static pthread_mutex_t openssl_init_mutex = NULL;
#else
static pthread_mutex_t openssl_init_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
static pthread_mutex_t *amqp_openssl_lockarray = NULL;
#endif /* ENABLE_THREAD_SAFETY */

struct amqp_ssl_socket_context {
	BIO *bio;
	SSL_CTX *ctx;
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
	ssize_t sent;
	struct amqp_ssl_socket_context *self = user_data;
	ERR_clear_error();
	sent = BIO_write(self->bio, buf, len);
	if (0 > sent) {
		SSL *ssl;
		int error;
		BIO_get_ssl(self->bio, &ssl);
		error = SSL_get_error(ssl, sent);
		switch (error) {
		case SSL_ERROR_NONE:
		case SSL_ERROR_ZERO_RETURN:
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			sent = 0;
			break;
		}
	}
	return sent;
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
	written = amqp_ssl_socket_send(sockfd, self->buffer, bytes, 0, self);
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
	ssize_t received;
	ERR_clear_error();
	received = BIO_read(self->bio, buf, len);
	if (0 > received) {
		SSL *ssl;
		int error;
		BIO_get_ssl(self->bio, &ssl);
		error = SSL_get_error(ssl, received);
		switch (error) {
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			received = 0;
			break;
		}
	}
	return received;
}

static int
amqp_ssl_socket_close(int sockfd,
		      void *user_data)
{
	struct amqp_ssl_socket_context *self = user_data;
	if (self) {
		BIO_free_all(self->bio);
		SSL_CTX_free(self->ctx);
		free(self->buffer);
		free(self);
	}
	destroy_openssl();
	return 0 > sockfd ? -1 : 0;
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
	SSL *ssl;
	X509 *peer;
	long result;
	X509_NAME *name;
	X509_NAME_ENTRY *entry;
	ASN1_STRING *entry_string;
	struct amqp_ssl_socket_context *self;
	int sockfd, status, pos, utf8_length;
	unsigned char *utf8_value = NULL, *cp, ch;
	initialize_openssl();
	self = calloc(1, sizeof(*self));
	if (!self) {
		goto error;
	}
	self->ctx = SSL_CTX_new(SSLv23_client_method());
	if (!self->ctx) {
		goto error;
	}
	status = SSL_CTX_load_verify_locations(self->ctx, cacert, NULL);
	if (1 != status) {
		goto error;
	}
	if (key && cert) {
		status = SSL_CTX_use_PrivateKey_file(self->ctx, key,
						     SSL_FILETYPE_PEM);
		if (1 != status) {
			goto error;
		}
		status = SSL_CTX_use_certificate_chain_file(self->ctx, cert);
		if (1 != status) {
			goto error;
		}
	}
	self->bio = BIO_new_ssl_connect(self->ctx);
	if (!self->bio) {
		goto error;
	}
	BIO_get_ssl(self->bio, &ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	BIO_set_conn_hostname(self->bio, host);
	BIO_set_conn_int_port(self->bio, &port);
	status = BIO_do_connect(self->bio);
	if (1 != status) {
		goto error;
	}
	result = SSL_get_verify_result(ssl);
	if (X509_V_OK != result) {
		goto error;
	}
	peer = SSL_get_peer_certificate(ssl);
	if (!peer) {
		goto error;
	}
	name = X509_get_subject_name(peer);
	if (!name) {
		goto error;
	}
	pos = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
	if (0 > pos) {
		goto error;
	}
	entry = X509_NAME_get_entry(name, pos);
	if (!entry) {
		goto error;
	}
	entry_string = X509_NAME_ENTRY_get_data(entry);
	if (!entry_string) {
		goto error;
	}
	utf8_length = ASN1_STRING_to_UTF8(&utf8_value, entry_string);
	if (0 > utf8_length) {
		goto error;
	}
	while (utf8_length > 0 && utf8_value[utf8_length - 1] == 0) {
		--utf8_length;
	}
	if (utf8_length >= 256) {
		goto error;
	}
	if ((size_t)utf8_length != strlen((char *)utf8_value)) {
		goto error;
	}
	for (cp = utf8_value; (ch = *cp) != '\0'; ++cp) {
		if (isascii(ch) && !isprint(ch)) {
			goto error;
		}
	}
#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif
	if (strcasecmp(host, (char *)utf8_value)) {
		goto error;
	}
#ifdef _MSC_VER
#undef strcasecmp
#endif
	sockfd = BIO_get_fd(self->bio, NULL);
	amqp_set_sockfd_full(state, sockfd,
			     amqp_ssl_socket_writev,
			     amqp_ssl_socket_send,
			     amqp_ssl_socket_recv,
			     amqp_ssl_socket_close,
			     amqp_ssl_socket_error,
			     self);
exit:
	OPENSSL_free(utf8_value);
	return sockfd;
error:
	OPENSSL_free(utf8_value);
	amqp_ssl_socket_close(-1, self);
	sockfd = -1;
	goto exit;
}

void
amqp_set_initialize_ssl_library(amqp_boolean_t do_initialize)
{
  if (!openssl_initialized)
  {
    do_initialize_openssl = do_initialize;
  }
}


#ifdef ENABLE_THREAD_SAFETY
unsigned long
amqp_ssl_threadid_callback(void)
{
  return (unsigned long)pthread_self();
}

void
amqp_ssl_locking_callback(int mode, int n,
			  AMQP_UNUSED const char *file,
			  AMQP_UNUSED int line)
{
  if (mode & CRYPTO_LOCK)
  {
    if (pthread_mutex_lock(&amqp_openssl_lockarray[n]))
      amqp_abort("Runtime error: Failure in trying to lock OpenSSL mutex");
  }
  else
  {
    if (pthread_mutex_unlock(&amqp_openssl_lockarray[n]))
      amqp_abort("Runtime error: Failure in trying to unlock OpenSSL mutex");
  }
}
#endif /* ENABLE_THREAD_SAFETY */

static int
initialize_openssl(void)
{
#ifdef _WIN32
  /* No such thing as PTHREAD_INITIALIZE_MUTEX macro on Win32, so we use this */
  if (NULL == openssl_init_mutex)
  {
    while (InterlockedExchange(&win32_create_mutex, 1) == 1)
      /* Loop, someone else is holding this lock */ ;

    if (NULL == openssl_init_mutex)
    {
      if (pthread_mutex_init(&openssl_init_mutex, NULL))
        return -1;
    }
    InterlockedExchange(&win32_create_mutex, 0);
  }
#endif /* _WIN32 */

#ifdef ENABLE_THREAD_SAFETY
  if (pthread_mutex_lock(&openssl_init_mutex))
    return -1;
#endif /* ENABLE_THREAD_SAFETY */
  if (do_initialize_openssl)
  {
#ifdef ENABLE_THREAD_SAFETY
    if (NULL == amqp_openssl_lockarray)
    {
      int i = 0;
      amqp_openssl_lockarray = calloc(CRYPTO_num_locks(), sizeof(pthread_mutex_t));
      if (!amqp_openssl_lockarray)
      {
        pthread_mutex_unlock(&openssl_init_mutex);
        return -1;
      }
      for (i = 0; i < CRYPTO_num_locks(); ++i)
      {
        if (pthread_mutex_init(&amqp_openssl_lockarray[i], NULL))
        {
          free(amqp_openssl_lockarray);
          amqp_openssl_lockarray = NULL;
          pthread_mutex_unlock(&openssl_init_mutex);
          return -1;
        }
      }
    }

    if (0 == open_ssl_connections)
    {
      CRYPTO_set_id_callback(amqp_ssl_threadid_callback);
      CRYPTO_set_locking_callback(amqp_ssl_locking_callback);
    }
#endif /* ENABLE_THREAD_SAFETY */

    if (!openssl_initialized)
    {
      OPENSSL_config(NULL);

      SSL_library_init();
      SSL_load_error_strings();

      openssl_initialized = 1;
    }
  }

  ++open_ssl_connections;

#ifdef ENABLE_THREAD_SAFETY
  pthread_mutex_unlock(&openssl_init_mutex);
#endif /* ENABLE_THREAD_SAFETY */
  return 0;
}

static int
destroy_openssl(void)
{
#ifdef ENABLE_THREAD_SAFETY
  if (pthread_mutex_lock(&openssl_init_mutex))
    return -1;
#endif /* ENABLE_THREAD_SAFETY */

  if (open_ssl_connections > 0)
    --open_ssl_connections;

#ifdef ENABLE_THREAD_SAFETY
  if (0 == open_ssl_connections && do_initialize_openssl)
  {
    /* Unsetting these allows the rabbitmq-c library to be unloaded
     * safely. We do leak the amqp_openssl_lockarray. Which is only
     * an issue if you repeatedly unload and load the library
     */
    CRYPTO_set_locking_callback(NULL);
    CRYPTO_set_id_callback(NULL);
  }

  pthread_mutex_unlock(&openssl_init_mutex);
#endif /* ENABLE_THREAD_SAFETY */
  return 0;
}
