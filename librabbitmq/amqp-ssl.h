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

/**
 * \file
 * Open an SSL/TLS connection
 */

#ifndef AMQP_SSL_H
#define AMQP_SSL_H

#include <amqp.h>

/**
 * Open an SSL connection to an AMQP broker.
 *
 * This function will setup an AMQP connection state object for SSL/TLS
 * communication. The caller of this function should not use the returned
 * file descriptor as input to amqp_set_sockfd() or amqp_set_sockfd_full(),
 * or directly for network I/O.
 *
 * If a client key or certificate file is provide then they should both be
 * provided.
 *
 * \param [in,out] state An AMQP connection state object.
 * \param [in] host The name of the host to connect to.
 * \param [in] port The port to connect on.
 * \param [in] caert Path the CA cert file in PEM format.
 * \param [in] key Path to the client key in PEM format. (may be NULL)
 * \param [in] cert Path to the client cert in PEM format. (may be NULL)
 *
 * \return A socket file-descriptor (-1 if an error occurred).
 */
AMQP_PUBLIC_FUNCTION
int
amqp_open_ssl_socket(amqp_connection_state_t state,
		     const char *host,
		     int port,
		     const char *cacert,
		     const char *key,
		     const char *cert);

/**
 * Sets whether rabbitmq-c initializes the underlying SSL library
 *
 * For SSL libraries that require a one-time initialization across 
 * a whole program (e.g., OpenSSL) this sets whether or not rabbitmq-c
 * will initialize the SSL library when the first call to 
 * amqp_open_ssl_socket() is made. You should call this function with
 * do_init = 0 if the underlying SSL library is intialized somewhere else
 * the program. 
 *
 * Failing to initialize or double initialization of the SSL library will 
 * result in undefined behavior
 *
 * By default rabbitmq-c will initialize the underlying SSL library
 *
 * NOTE: calling this function after the first socket has been opened with
 * amqp_open_ssl_socket() will not have any effect.
 *
 * \param [in] do_initalize, if 0 rabbitmq-c will not initialize the SSL library,
 *              otherwise rabbitmq-c will initialize the SSL library
 * 
 */
AMQP_PUBLIC_FUNCTION
void
amqp_set_initialize_ssl_library(amqp_boolean_t do_initialize);

#endif /* AMQP_SSL_H */
