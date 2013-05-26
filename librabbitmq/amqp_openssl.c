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

#include "amqp_ssl_socket.h"
#include "amqp_private.h"
#include "threads.h"
#include <ctype.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdlib.h>
#include <string.h>

#include "socket.h"

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

struct amqp_ssl_socket_t {
  const struct amqp_socket_class_t *klass;
  SSL_CTX *ctx;
  int sockfd;
  SSL *ssl;
  char *buffer;
  size_t length;
  amqp_boolean_t verify;
  int last_error;
};

static ssize_t
amqp_ssl_socket_send(void *base,
                     const void *buf,
                     size_t len,
                     AMQP_UNUSED int flags)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  ssize_t sent;
  ERR_clear_error();
  self->last_error = 0;
  sent = SSL_write(self->ssl, buf, len);
  if (0 > sent) {
    self->last_error = AMQP_STATUS_SSL_ERROR;
    switch (SSL_get_error(self->ssl, sent)) {
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
amqp_ssl_socket_writev(void *base,
                       const struct iovec *iov,
                       int iovcnt)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  ssize_t written = -1;
  char *bufferp;
  size_t bytes;
  int i;
  self->last_error = 0;
  bytes = 0;
  for (i = 0; i < iovcnt; ++i) {
    bytes += iov[i].iov_len;
  }
  if (self->length < bytes) {
    free(self->buffer);
    self->buffer = malloc(bytes);
    if (!self->buffer) {
      self->length = 0;
      self->last_error = AMQP_STATUS_NO_MEMORY;
      goto exit;
    }
    self->length = bytes;
  }
  bufferp = self->buffer;
  for (i = 0; i < iovcnt; ++i) {
    memcpy(bufferp, iov[i].iov_base, iov[i].iov_len);
    bufferp += iov[i].iov_len;
  }
  written = amqp_ssl_socket_send(self, self->buffer, bytes, 0);
exit:
  return written;
}

static ssize_t
amqp_ssl_socket_recv(void *base,
                     void *buf,
                     size_t len,
                     AMQP_UNUSED int flags)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  ssize_t received;
  ERR_clear_error();
  self->last_error = 0;
  received = SSL_read(self->ssl, buf, len);
  if (0 > received) {
    self->last_error = AMQP_STATUS_SSL_ERROR;
    switch(SSL_get_error(self->ssl, received)) {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      received = 0;
      break;
    }
  }
  return received;
}

static int
amqp_ssl_socket_verify(void *base, const char *host)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  unsigned char *utf8_value = NULL, *cp, ch;
  int pos, utf8_length, status = 0;
  ASN1_STRING *entry_string;
  X509_NAME_ENTRY *entry;
  X509_NAME *name;
  X509 *peer;
  peer = SSL_get_peer_certificate(self->ssl);
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
exit:
  OPENSSL_free(utf8_value);
  return status;
error:
  status = -1;
  goto exit;
}

static int
amqp_ssl_socket_open(void *base, const char *host, int port)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  long result;
  int status;
  self->last_error = 0;
  self->ssl = SSL_new(self->ctx);
  if (!self->ssl) {
    self->last_error = AMQP_STATUS_SSL_ERROR;
    return -1;
  }
  SSL_set_mode(self->ssl, SSL_MODE_AUTO_RETRY);
  self->sockfd = amqp_open_socket(host, port);
  if (0 > self->sockfd) {
    self->last_error = -self->sockfd;
    return -1;
  }
  status = SSL_set_fd(self->ssl, self->sockfd);
  if (!status) {
    self->last_error = AMQP_STATUS_SSL_ERROR;
    return -1;
  }
  status = SSL_connect(self->ssl);
  if (!status) {
    self->last_error = AMQP_STATUS_SSL_ERROR;
    return -1;
  }
  result = SSL_get_verify_result(self->ssl);
  if (X509_V_OK != result) {
    self->last_error = AMQP_STATUS_SSL_ERROR;
    return -1;
  }
  if (self->verify) {
    int status = amqp_ssl_socket_verify(self, host);
    if (status) {
      self->last_error = AMQP_STATUS_SSL_ERROR;
      return -1;
    }
  }
  return 0;
}

static int
amqp_ssl_socket_close(void *base)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  if (self) {
    SSL_free(self->ssl);
    amqp_os_socket_close(self->sockfd);
    SSL_CTX_free(self->ctx);
    free(self->buffer);
    free(self);
  }
  destroy_openssl();
  return 0;
}

static int
amqp_ssl_socket_error(void *base)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  return self->last_error;
}

char *
amqp_ssl_error_string(AMQP_UNUSED int err)
{
  return strdup("A ssl socket error occurred.");
}

static int
amqp_ssl_socket_get_sockfd(void *base)
{
  struct amqp_ssl_socket_t *self = (struct amqp_ssl_socket_t *)base;
  return self->sockfd;
}

static const struct amqp_socket_class_t amqp_ssl_socket_class = {
  amqp_ssl_socket_writev, /* writev */
  amqp_ssl_socket_send, /* send */
  amqp_ssl_socket_recv, /* recv */
  amqp_ssl_socket_open, /* open */
  amqp_ssl_socket_close, /* close */
  amqp_ssl_socket_error, /* error */
  amqp_ssl_socket_get_sockfd /* get_sockfd */
};

amqp_socket_t *
amqp_ssl_socket_new(void)
{
  struct amqp_ssl_socket_t *self = calloc(1, sizeof(*self));
  int status;
  if (!self) {
    goto error;
  }
  status = initialize_openssl();
  if (status) {
    goto error;
  }
  self->ctx = SSL_CTX_new(SSLv23_client_method());
  if (!self->ctx) {
    goto error;
  }
  self->klass = &amqp_ssl_socket_class;
  self->verify = 1;
  return (amqp_socket_t *)self;
error:
  amqp_socket_close((amqp_socket_t *)self);
  return NULL;
}

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
  status = SSL_CTX_load_verify_locations(self->ctx, cacert, NULL);
  if (1 != status) {
    return -1;
  }
  return 0;
}

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
  status = SSL_CTX_use_certificate_chain_file(self->ctx, cert);
  if (1 != status) {
    return -1;
  }
  status = SSL_CTX_use_PrivateKey_file(self->ctx, key,
                                       SSL_FILETYPE_PEM);
  if (1 != status) {
    return -1;
  }
  return 0;
}

static int
password_cb(AMQP_UNUSED char *buffer,
            AMQP_UNUSED int length,
            AMQP_UNUSED int rwflag,
            AMQP_UNUSED void *user_data)
{
  amqp_abort("rabbitmq-c does not support password protected keys");
  return 0;
}

int
amqp_ssl_socket_set_key_buffer(amqp_socket_t *base,
                               const char *cert,
                               const void *key,
                               size_t n)
{
  int status = 0;
  BIO *buf = NULL;
  RSA *rsa = NULL;
  struct amqp_ssl_socket_t *self;
  if (base->klass != &amqp_ssl_socket_class) {
    amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
  }
  self = (struct amqp_ssl_socket_t *)base;
  status = SSL_CTX_use_certificate_chain_file(self->ctx, cert);
  if (1 != status) {
    return -1;
  }
  buf = BIO_new_mem_buf((void *)key, n);
  if (!buf) {
    goto error;
  }
  rsa = PEM_read_bio_RSAPrivateKey(buf, NULL, password_cb, NULL);
  if (!rsa) {
    goto error;
  }
  status = SSL_CTX_use_RSAPrivateKey(self->ctx, rsa);
  if (1 != status) {
    goto error;
  }
exit:
  BIO_vfree(buf);
  RSA_free(rsa);
  return status;
error:
  status = -1;
  goto exit;
}

int
amqp_ssl_socket_set_cert(amqp_socket_t *base,
                         const char *cert)
{
  int status;
  struct amqp_ssl_socket_t *self;
  if (base->klass != &amqp_ssl_socket_class) {
    amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
  }
  self = (struct amqp_ssl_socket_t *)base;
  status = SSL_CTX_use_certificate_chain_file(self->ctx, cert);
  if (1 != status) {
    return -1;
  }
  return 0;
}

void
amqp_ssl_socket_set_verify(amqp_socket_t *base,
                           amqp_boolean_t verify)
{
  struct amqp_ssl_socket_t *self;
  if (base->klass != &amqp_ssl_socket_class) {
    amqp_abort("<%p> is not of type amqp_ssl_socket_t", base);
  }
  self = (struct amqp_ssl_socket_t *)base;
  self->verify = verify;
}

void
amqp_set_initialize_ssl_library(amqp_boolean_t do_initialize)
{
  if (!openssl_initialized) {
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
  if (mode & CRYPTO_LOCK) {
    if (pthread_mutex_lock(&amqp_openssl_lockarray[n])) {
      amqp_abort("Runtime error: Failure in trying to lock OpenSSL mutex");
    }
  } else {
    if (pthread_mutex_unlock(&amqp_openssl_lockarray[n])) {
      amqp_abort("Runtime error: Failure in trying to unlock OpenSSL mutex");
    }
  }
}
#endif /* ENABLE_THREAD_SAFETY */

static int
initialize_openssl(void)
{
#ifdef _WIN32
  /* No such thing as PTHREAD_INITIALIZE_MUTEX macro on Win32, so we use this */
  if (NULL == openssl_init_mutex) {
    while (InterlockedExchange(&win32_create_mutex, 1) == 1)
      /* Loop, someone else is holding this lock */ ;

    if (NULL == openssl_init_mutex) {
      if (pthread_mutex_init(&openssl_init_mutex, NULL)) {
        return -1;
      }
    }
    InterlockedExchange(&win32_create_mutex, 0);
  }
#endif /* _WIN32 */

#ifdef ENABLE_THREAD_SAFETY
  if (pthread_mutex_lock(&openssl_init_mutex)) {
    return -1;
  }
#endif /* ENABLE_THREAD_SAFETY */
  if (do_initialize_openssl) {
#ifdef ENABLE_THREAD_SAFETY
    if (NULL == amqp_openssl_lockarray) {
      int i = 0;
      amqp_openssl_lockarray = calloc(CRYPTO_num_locks(), sizeof(pthread_mutex_t));
      if (!amqp_openssl_lockarray) {
        pthread_mutex_unlock(&openssl_init_mutex);
        return -1;
      }
      for (i = 0; i < CRYPTO_num_locks(); ++i) {
        if (pthread_mutex_init(&amqp_openssl_lockarray[i], NULL)) {
          free(amqp_openssl_lockarray);
          amqp_openssl_lockarray = NULL;
          pthread_mutex_unlock(&openssl_init_mutex);
          return -1;
        }
      }
    }

    if (0 == open_ssl_connections) {
      CRYPTO_set_id_callback(amqp_ssl_threadid_callback);
      CRYPTO_set_locking_callback(amqp_ssl_locking_callback);
    }
#endif /* ENABLE_THREAD_SAFETY */

    if (!openssl_initialized) {
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
  if (pthread_mutex_lock(&openssl_init_mutex)) {
    return -1;
  }
#endif /* ENABLE_THREAD_SAFETY */

  if (open_ssl_connections > 0) {
    --open_ssl_connections;
  }

#ifdef ENABLE_THREAD_SAFETY
  if (0 == open_ssl_connections && do_initialize_openssl) {
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
