/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
/*
 * Portions created by Alan Antonuk are Copyright (c) 2013-2014 Alan Antonuk.
 * All Rights Reserved.
 *
 * Portions created by Michael Steinert are Copyright (c) 2012-2013 Michael
 * Steinert. All Rights Reserved.
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

#include "amqp_private.h"
#include "amqp_time.h"

AMQP_BEGIN_DECLS

typedef enum {
  AMQP_SF_NONE = 0,
  AMQP_SF_MORE = 1,
  AMQP_SF_POLLIN = 2,
  AMQP_SF_POLLOUT = 4,
  AMQP_SF_POLLERR = 8
} amqp_socket_flag_enum;

typedef enum {
  AMQP_SC_NONE = 0,
  AMQP_SC_FORCE = 1
} amqp_socket_close_enum;

int
amqp_os_socket_error(void);

int
amqp_os_socket_close(int sockfd);

/* Socket callbacks. */
typedef ssize_t (*amqp_socket_send_fn)(void *, const void *, size_t, int);
typedef ssize_t (*amqp_socket_recv_fn)(void *, void *, size_t, int);
typedef int (*amqp_socket_open_fn)(void *, const char *, int, struct timeval *);
typedef int (*amqp_socket_close_fn)(void *, amqp_socket_close_enum);
typedef int (*amqp_socket_get_sockfd_fn)(void *);
typedef void (*amqp_socket_delete_fn)(void *);

/** V-table for amqp_socket_t */
struct amqp_socket_class_t {
  amqp_socket_send_fn send;
  amqp_socket_recv_fn recv;
  amqp_socket_open_fn open;
  amqp_socket_close_fn close;
  amqp_socket_get_sockfd_fn get_sockfd;
  amqp_socket_delete_fn delete;
};

/** Abstract base class for amqp_socket_t */
struct amqp_socket_t_ {
  const struct amqp_socket_class_t *klass;
};


/**
 * Set set the socket object for a connection
 *
 * This assigns a socket object to the connection, closing and deleting any
 * existing socket
 *
 * \param [in] state The connection object to add the socket to
 * \param [in] socket The socket object to assign to the connection
 */
void
amqp_set_socket(amqp_connection_state_t state, amqp_socket_t *socket);


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
 * \param [in]
 *
 * \return AMQP_STATUS_OK on success. amqp_status_enum value otherwise
 */
ssize_t
amqp_socket_send(amqp_socket_t *self, const void *buf, size_t len, int flags);

ssize_t amqp_try_send(amqp_connection_state_t state, const void *buf,
                      size_t len, amqp_time_t deadline, int flags);

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

/**
 * Close a socket connection and free resources.
 *
 * This function closes a socket connection and releases any resources used by
 * the object. After calling this function the specified socket should no
 * longer be referenced.
 *
 * \param [in,out] self A socket object.
 * \param [in] force, if set, just close the socket, don't attempt a TLS
 * shutdown.
 *
 * \return Zero upon success, non-zero otherwise.
 */
int
amqp_socket_close(amqp_socket_t *self, amqp_socket_close_enum force);

/**
 * Destroy a socket object
 *
 * \param [in] self the socket object to delete
 */
void
amqp_socket_delete(amqp_socket_t *self);

/**
 * Open a socket connection.
 *
 * This function opens a socket connection returned from amqp_tcp_socket_new()
 * or amqp_ssl_socket_new(). This function should be called after setting
 * socket options and prior to assigning the socket to an AMQP connection with
 * amqp_set_socket().
 *
 * \param [in] host Connect to this host.
 * \param [in] port Connect on this remote port.
 * \param [in] timeout Max allowed time to spent on opening. If NULL - run in blocking mode
 *
 * \return File descriptor upon success, non-zero negative error code otherwise.
 */
int
amqp_open_socket_noblock(char const *hostname, int portnumber, struct timeval *timeout);

int amqp_open_socket_inner(char const *hostname, int portnumber,
                           amqp_time_t deadline);

/* Wait up to dealline for fd to become readable or writeable depending on
 * event (AMQP_SF_POLLIN, AMQP_SF_POLLOUT) */
int amqp_poll(int fd, int event, amqp_time_t deadline);


/**
 * Waits for a specific method from the broker with a timeout
 *
 * \warning You probably don't want to use this function. If this function
 *  doesn't receive exactly the frame requested it closes the whole connection.
 *
 * Waits for a single method on a channel from the broker.
 * If a frame is received that does not match expected_channel
 * or expected_method the program will abort
 *
 * \param [in] state the connection object
 * \param [in] expected_channel the channel that the method should be delivered on
 * \param [in] expected_method the method to wait for
 * \param [in] timeout a timeout to wait for a method. Passing in
 *             NULL will result in blocking behavior.
 * \param [out] output the method
 * \returns AMQP_STATUS_OK on success. An amqp_status_enum value is returned
 *  otherwise. Possible errors include:
 *  - AMQP_STATUS_WRONG_METHOD a frame containing the wrong method, wrong frame
 *    type or wrong channel was received. The connection is closed.
 *  - AMQP_STATUS_NO_MEMORY failure in allocating memory. The library is likely in
 *    an indeterminate state making recovery unlikely. Client should note the error
 *    and terminate the application
 *  - AMQP_STATUS_BAD_AMQP_DATA bad AMQP data was received. The connection
 *    should be shutdown immediately
 *  - AMQP_STATUS_UNKNOWN_METHOD: an unknown method was received from the
 *    broker. This is likely a protocol error and the connection should be
 *    shutdown immediately
 *  - AMQP_STATUS_UNKNOWN_CLASS: a properties frame with an unknown class
 *    was received from the broker. This is likely a protocol error and the
 *    connection should be shutdown immediately
 *  - AMQP_STATUS_HEARTBEAT_TIMEOUT timed out while waiting for heartbeat
 *    from the broker. The connection has been closed.
 *  - AMQP_STATUS_TIMEOUT the timeout was reached while waiting for a method.
 *  - AMQP_STATUS_TIMER_FAILURE system timer indicated failure.
 *  - AMQP_STATUS_SOCKET_ERROR a socket error occurred. The connection has
 *    been closed
 *  - AMQP_STATUS_SSL_ERROR a SSL socket error occurred. The connection has
 *    been closed.
 */
int amqp_simple_wait_method_noblock(amqp_connection_state_t state,
                                    amqp_channel_t expected_channel,
                                    amqp_method_number_t expected_method,
                                    struct timeval * timeout,
                                    amqp_method_t *output);


/**
 * Sends a method to the broker and waits for a method response
 *
 * \param [in] state the connection object
 * \param [in] channel the channel object
 * \param [in] request_id the method number of the request
 * \param [in] expected_reply_ids a 0 terminated array of expected response
 *             method numbers
 * \param [in] decoded_request_method the method to be sent to the broker
 * \param [in] timeout a timeout to wait for a RPC answer. Passing in
 *             NULL will result in blocking behavior.
 * \return a amqp_rpc_reply_t:
 *  - r.reply_type == AMQP_RESPONSE_NORMAL. RPC completed successfully
 *  - r.reply_type == AMQP_STATUS_TIMEOUT the timeout was reached while waiting
 *    for RPC answer.
 *  - r.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION. The broker returned an
 *    exception:
 *    - If r.reply.id == AMQP_CHANNEL_CLOSE_METHOD a channel exception
 *      occurred, cast r.reply.decoded to amqp_channel_close_t* to see details
 *      of the exception. The client should amqp_send_method() a
 *      amqp_channel_close_ok_t. The channel must be re-opened before it
 *      can be used again. Any resources associated with the channel
 *      (auto-delete exchanges, auto-delete queues, consumers) are invalid
 *      and must be recreated before attempting to use them again.
 *    - If r.reply.id == AMQP_CONNECTION_CLOSE_METHOD a connection exception
 *      occurred, cast r.reply.decoded to amqp_connection_close_t* to see
 *      details of the exception. The client amqp_send_method() a
 *      amqp_connection_close_ok_t and disconnect from the broker.
 *  - r.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION. An exception occurred
 *    within the library. Examine r.library_error and compare it against
 *    amqp_status_enum values to determine the error.
 */
amqp_rpc_reply_t
AMQP_CALL amqp_simple_rpc_noblock(amqp_connection_state_t state,
                                  amqp_channel_t channel,
                                  amqp_method_number_t request_id,
                                  amqp_method_number_t *expected_reply_ids,
                                  void *decoded_request_method,
                                  struct timeval *timeout);


/**
 * Sends a method to the broker and waits for a method response
 *
 * \param [in] state the connection object
 * \param [in] channel the channel object
 * \param [in] request_id the method number of the request
 * \param [in] reply_id the method number expected in response
 * \param [in] decoded_request_method the request method
 * \param [in] timeout a timeout to wait for a RPC answer. Passing in
 *             NULL will result in blocking behavior.
 * \return a pointer to the method returned from the broker, or NULL on error.
 *  On error amqp_get_rpc_reply() will return an amqp_rpc_reply_t with
 *  details on the error that occurred.
 */
void *
amqp_simple_rpc_decoded_noblock(amqp_connection_state_t state,
                                amqp_channel_t channel,
                                amqp_method_number_t request_id,
                                amqp_method_number_t reply_id,
                                void *decoded_request_method,
                                struct timeval *timeout);

int amqp_send_method_inner_noblock(amqp_connection_state_t state,
                                   amqp_channel_t channel,
                                   amqp_method_number_t id,
                                   void *decoded, int flags,
                                   struct timeval *timeout);

int
amqp_queue_frame(amqp_connection_state_t state, amqp_frame_t *frame);

int
amqp_put_back_frame(amqp_connection_state_t state, amqp_frame_t *frame);

int
amqp_simple_wait_frame_on_channel(amqp_connection_state_t state,
                                  amqp_channel_t channel,
                                  amqp_frame_t *decoded_frame);

int
sasl_mechanism_in_list(amqp_bytes_t mechanisms, amqp_sasl_method_enum method);

int amqp_merge_capabilities(const amqp_table_t *base, const amqp_table_t *add,
                            amqp_table_t *result, amqp_pool_t *pool);
AMQP_END_DECLS

#endif /* AMQP_SOCKET_H */
