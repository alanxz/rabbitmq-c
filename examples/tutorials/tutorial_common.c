// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include "tutorial_common.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef AMQP_TUTORIALS_HAVE_SSL
#include <rabbitmq-c/ssl_socket.h>
#endif
#include <rabbitmq-c/tcp_socket.h>

#define DEFAULT_URL "amqp://guest:guest@localhost:5672/%2F"
#define MAX_FRAME_SIZE 131072
/* Ask the broker to send heartbeats this often (seconds). If the broker or the
 * network disappears, the library notices after ~2 missed heartbeats instead
 * of hanging forever. */
#define HEARTBEAT_SECONDS 30
#define CONNECT_TIMEOUT_SECONDS 10

static volatile sig_atomic_t interrupted = 0;

static void on_signal(int sig) {
  (void)sig;
  interrupted = 1;
}

void tutorial_install_signal_handlers(void) {
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);
}

int tutorial_interrupted(void) { return interrupted; }

const char *tutorial_broker_url(int argc, char const *const *argv) {
  const char *env;
  if (argc > 1) {
    return argv[1];
  }
  env = getenv("RABBITMQ_URL");
  return (env != NULL && env[0] != '\0') ? env : DEFAULT_URL;
}

int tutorial_check_status(int status, char const *context) {
  if (status < 0) {
    fprintf(stderr, "%s: %s\n", context, amqp_error_string2(status));
    return TUTORIAL_ERROR;
  }
  return TUTORIAL_OK;
}

int tutorial_check_reply(amqp_rpc_reply_t reply, char const *context) {
  switch (reply.reply_type) {
    case AMQP_RESPONSE_NORMAL:
      return TUTORIAL_OK;

    case AMQP_RESPONSE_NONE:
      fprintf(stderr, "%s: missing RPC reply type\n", context);
      break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      fprintf(stderr, "%s: %s\n", context,
              amqp_error_string2(reply.library_error));
      break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
      switch (reply.reply.id) {
        case AMQP_CONNECTION_CLOSE_METHOD: {
          amqp_connection_close_t *m =
              (amqp_connection_close_t *)reply.reply.decoded;
          fprintf(stderr, "%s: broker closed the connection: %u %.*s\n",
                  context, (unsigned)m->reply_code, (int)m->reply_text.len,
                  (char *)m->reply_text.bytes);
          break;
        }
        case AMQP_CHANNEL_CLOSE_METHOD: {
          amqp_channel_close_t *m = (amqp_channel_close_t *)reply.reply.decoded;
          fprintf(stderr, "%s: broker closed the channel: %u %.*s\n", context,
                  (unsigned)m->reply_code, (int)m->reply_text.len,
                  (char *)m->reply_text.bytes);
          break;
        }
        default:
          fprintf(stderr, "%s: unexpected broker reply 0x%08X\n", context,
                  (unsigned)reply.reply.id);
          break;
      }
      break;
  }
  return TUTORIAL_ERROR;
}

int tutorial_connect(const char *url, amqp_connection_state_t *conn_out) {
  struct amqp_connection_info info;
  char *url_copy = NULL;
  amqp_connection_state_t conn = NULL;
  amqp_socket_t *socket = NULL;
  struct timeval timeout;
  int status;

  *conn_out = NULL;

  /* amqp_parse_url() modifies its input and the parsed fields point into it,
   * so it needs a writable copy that outlives the login call below. */
  url_copy = strdup(url);
  if (url_copy == NULL) {
    fprintf(stderr, "Out of memory\n");
    return TUTORIAL_ERROR;
  }
  status = amqp_parse_url(url_copy, &info);
  if (status != AMQP_STATUS_OK) {
    fprintf(stderr, "Invalid broker URL: %s\n", amqp_error_string2(status));
    goto error;
  }

  conn = amqp_new_connection();
  if (conn == NULL) {
    fprintf(stderr, "Creating connection: out of memory\n");
    goto error;
  }

  if (info.ssl) {
#ifdef AMQP_TUTORIALS_HAVE_SSL
    socket = amqp_ssl_socket_new(conn);
    if (socket != NULL) {
      /* Verify the broker certificate and hostname; never turn this off in
       * production. See amqp_ssl_connect.c for custom CA configuration. */
      amqp_ssl_socket_set_verify_peer(socket, 1);
      amqp_ssl_socket_set_verify_hostname(socket, 1);
    }
#else
    fprintf(stderr, "This build of the examples has no TLS support\n");
    goto error;
#endif
  } else {
    socket = amqp_tcp_socket_new(conn);
  }
  if (socket == NULL) {
    fprintf(stderr, "Creating socket failed\n");
    goto error;
  }

  /* Bound the time spent connecting so a black-holed host can't hang us. */
  timeout.tv_sec = CONNECT_TIMEOUT_SECONDS;
  timeout.tv_usec = 0;
  status = amqp_socket_open_noblock(socket, info.host, info.port, &timeout);
  if (status != AMQP_STATUS_OK) {
    fprintf(stderr, "Opening socket to %s:%d: %s\n", info.host, info.port,
            amqp_error_string2(status));
    goto error;
  }

  if (tutorial_check_reply(
          amqp_login(conn, info.vhost, 0, MAX_FRAME_SIZE, HEARTBEAT_SECONDS,
                     AMQP_SASL_METHOD_PLAIN, info.user, info.password),
          "Logging in") != TUTORIAL_OK) {
    goto error;
  }

  amqp_channel_open(conn, TUTORIAL_CHANNEL);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Opening channel") !=
      TUTORIAL_OK) {
    goto error;
  }

  /* The login strings were copied by amqp_login(), the URL buffer is no longer
   * needed. */
  free(url_copy);
  *conn_out = conn;
  return TUTORIAL_OK;

error:
  /* The socket is owned by the connection state and is freed with it. */
  if (conn != NULL) {
    amqp_destroy_connection(conn);
  }
  free(url_copy);
  return TUTORIAL_ERROR;
}

void tutorial_close(amqp_connection_state_t conn) {
  if (conn == NULL) {
    return;
  }
  /* Best effort: if the connection is already dead these just fail fast. */
  amqp_channel_close(conn, TUTORIAL_CHANNEL, AMQP_REPLY_SUCCESS);
  amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
  amqp_destroy_connection(conn);
}

int tutorial_handle_unexpected_frame(amqp_connection_state_t conn) {
  amqp_frame_t frame;
  int status = amqp_simple_wait_frame(conn, &frame);

  if (tutorial_check_status(status, "Reading frame") != TUTORIAL_OK) {
    return TUTORIAL_ERROR;
  }
  if (frame.frame_type != AMQP_FRAME_METHOD) {
    return TUTORIAL_OK;
  }

  switch (frame.payload.method.id) {
    case AMQP_BASIC_ACK_METHOD:
    case AMQP_BASIC_NACK_METHOD:
      /* Publisher confirmation. Only arrives if confirm.select was used. */
      return TUTORIAL_OK;

    case AMQP_BASIC_RETURN_METHOD: {
      /* An unroutable mandatory message came back; its content follows. */
      amqp_message_t message;
      if (tutorial_check_reply(
              amqp_read_message(conn, frame.channel, &message, 0),
              "Reading returned message") != TUTORIAL_OK) {
        return TUTORIAL_ERROR;
      }
      fprintf(stderr, "Message was returned by the broker as unroutable\n");
      amqp_destroy_message(&message);
      return TUTORIAL_OK;
    }

    case AMQP_CHANNEL_CLOSE_METHOD: {
      /* Channel-level error (e.g. publishing to a missing exchange). Recovery
       * means opening a new channel and redoing declarations/consumers. */
      amqp_channel_close_t *m =
          (amqp_channel_close_t *)frame.payload.method.decoded;
      fprintf(stderr, "Broker closed the channel: %u %.*s\n",
              (unsigned)m->reply_code, (int)m->reply_text.len,
              (char *)m->reply_text.bytes);
      return TUTORIAL_ERROR;
    }

    case AMQP_CONNECTION_CLOSE_METHOD: {
      /* Connection-level error. Recovery means reconnecting from scratch. */
      amqp_connection_close_t *m =
          (amqp_connection_close_t *)frame.payload.method.decoded;
      fprintf(stderr, "Broker closed the connection: %u %.*s\n",
              (unsigned)m->reply_code, (int)m->reply_text.len,
              (char *)m->reply_text.bytes);
      return TUTORIAL_ERROR;
    }

    default:
      fprintf(stderr, "Unexpected method 0x%08X from broker\n",
              (unsigned)frame.payload.method.id);
      return TUTORIAL_ERROR;
  }
}
