// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * Helpers shared by the tutorial examples.
 *
 * Unlike the helpers in ../utils.h, none of these functions call exit().
 * Each one reports what went wrong on stderr and returns a status, so the
 * calling example can release resources on every path and choose its own
 * exit code. That is the pattern real applications should follow.
 */

#ifndef librabbitmq_examples_tutorial_common_h
#define librabbitmq_examples_tutorial_common_h

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#include <rabbitmq-c/amqp.h>

#define TUTORIAL_OK 0
#define TUTORIAL_ERROR (-1)

/* The only channel the tutorials use. */
#define TUTORIAL_CHANNEL 1

/*
 * Returns the broker URL: the first command line argument if present, else the
 * RABBITMQ_URL environment variable, else the RabbitMQ default
 * "amqp://guest:guest@localhost:5672/%2F".
 */
const char *tutorial_broker_url(int argc, char const *const *argv);

/*
 * Connects to the broker named by `url` (amqp:// or amqps://), logs in, opens
 * TUTORIAL_CHANNEL, and puts the connection into the state it should be in
 * before use. On success *conn_out owns the connection and must be released
 * with tutorial_close(). On failure *conn_out is NULL and nothing is leaked.
 */
int tutorial_connect(const char *url, amqp_connection_state_t *conn_out);

/*
 * Closes the channel and connection politely, then destroys the connection
 * state. Safe to call with NULL, and safe to call on a connection that is
 * already broken: failures during the shutdown handshake are ignored because
 * there is nothing useful left to do about them.
 */
void tutorial_close(amqp_connection_state_t conn);

/*
 * Inspects the reply of the last synchronous AMQP method call. Returns
 * TUTORIAL_OK if it succeeded; otherwise prints a message that includes
 * `context` and returns TUTORIAL_ERROR.
 */
int tutorial_check_reply(amqp_rpc_reply_t reply, char const *context);

/* Same as above for functions that return an amqp_status_enum or errno-style
 * negative value, e.g. amqp_basic_publish(). */
int tutorial_check_status(int status, char const *context);

/*
 * To be called when amqp_consume_message() returned
 * AMQP_RESPONSE_LIBRARY_EXCEPTION with AMQP_STATUS_UNEXPECTED_STATE, meaning
 * the next frame on the wire is not a message delivery. Reads that frame and
 * decides whether the session can continue.
 *
 * Returns TUTORIAL_OK if the frame was benign and consuming can continue,
 * TUTORIAL_ERROR if the channel or connection was closed by the broker (the
 * reason is printed).
 */
int tutorial_handle_unexpected_frame(amqp_connection_state_t conn);

typedef enum {
  TUTORIAL_WAIT_MESSAGE, /* *envelope is filled in; caller must destroy it */
  TUTORIAL_WAIT_IDLE,    /* nothing to process right now, call again */
  TUTORIAL_WAIT_FAILED   /* the session is unusable; reason already printed */
} tutorial_wait_result;

/*
 * Waits up to `timeout_seconds` for the next message delivery on the
 * connection. Wraps amqp_consume_message() and the error classification every
 * consumer needs: timeouts and benign non-delivery frames become
 * TUTORIAL_WAIT_IDLE, and anything that means the channel or connection is gone
 * becomes TUTORIAL_WAIT_FAILED. Also releases buffers from the previous
 * message.
 */
tutorial_wait_result tutorial_wait_for_message(amqp_connection_state_t conn,
                                               amqp_envelope_t *envelope,
                                               int timeout_seconds);

/* Sleeps for the given number of milliseconds. */
void tutorial_sleep_ms(unsigned int ms);

/*
 * Installs a SIGINT/SIGTERM handler that sets the flag returned by
 * tutorial_interrupted(). Long running examples poll it so they can shut down
 * cleanly instead of being killed in the middle of a protocol exchange.
 */
void tutorial_install_signal_handlers(void);
int tutorial_interrupted(void);

#endif
