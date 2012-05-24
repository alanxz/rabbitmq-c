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

#ifndef AMQP_SSL_H
#define AMQP_SSL_H

#include <amqp.h>

/**
 * \brief Open an SSL connection to an AMQP broker.
 *
 * If successful this function will setup the AMQP connection state object
 * for SSL/TLS communication. The caller of this function should not call
 * amqp_set_sockfd() or amqp_set_sockfd_full() after calling this function,
 * nor should the returned file descriptor be used directly for network I/O.
 *
 * \param state [in/out] An AMQP connection state object.
 * \param host [in] The name of the host to connect to.
 * \param port [in] The port to connect on.
 * \param caert [in] Path the CA cert file in PEM format.
 * \param key [in] Path to the client key in PEM format. (may be NULL)
 * \param cert [in] Path to the client cert in PEM format. (may be NULL)
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

#endif /* AMQP_SSL_H */
