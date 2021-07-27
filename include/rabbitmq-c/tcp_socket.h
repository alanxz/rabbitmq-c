// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/** \file */

/**
 * A TCP socket connection.
 */

#ifndef RABBITMQ_C_TCP_SOCKET_H
#define RABBITMQ_C_TCP_SOCKET_H

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/export.h>

AMQP_BEGIN_DECLS

/**
 * Create a new TCP socket.
 *
 * Call amqp_connection_close() to release socket resources.
 *
 * \return A new socket object or NULL if an error occurred.
 *
 * \since v0.4.0
 */
AMQP_EXPORT
amqp_socket_t *AMQP_CALL amqp_tcp_socket_new(amqp_connection_state_t state);

/**
 * Assign an open file descriptor to a socket object.
 *
 * This function must not be used in conjunction with amqp_socket_open(), i.e.
 * the socket connection should already be open(2) when this function is
 * called.
 *
 * \param [in,out] self A TCP socket object.
 * \param [in] sockfd An open socket descriptor.
 *
 * \since v0.4.0
 */
AMQP_EXPORT
void AMQP_CALL amqp_tcp_socket_set_sockfd(amqp_socket_t *self, int sockfd);

AMQP_END_DECLS

#endif /* RABBITMQ_C_TCP_SOCKET_H */
