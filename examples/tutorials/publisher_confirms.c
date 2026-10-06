// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

/*
 * RabbitMQ tutorial 7 ("Publisher Confirms").
 * https://www.rabbitmq.com/tutorials/tutorial-seven-java
 *
 * Without confirms, amqp_basic_publish() returning success only means the bytes
 * were written to the socket. With confirms the broker acknowledges (basic.ack)
 * each message once it has taken responsibility for it, or rejects it
 * (basic.nack) if it could not. This example publishes messages in batches and
 * waits for every message in a batch to be confirmed before sending the next,
 * which bounds how many messages can be in doubt at once.
 *
 * Handles all the outcomes a publisher must be prepared for:
 *  - ack (single and "multiple"): the message is safe
 *  - nack: the broker could not handle it; the caller decides whether to retry
 *  - basic.return: mandatory message was unroutable (an ack still follows)
 *  - no confirmation within the timeout, or the channel/connection closing
 *
 * Usage: publisher_confirms [amqp-url] [message-count]
 */

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>

#include "tutorial_common.h"

#define QUEUE_NAME "confirm_queue"
#define BATCH_SIZE 100
#define DEFAULT_MESSAGE_COUNT 1000
#define CONFIRM_TIMEOUT_SECONDS 10

typedef struct {
  uint64_t published; /* delivery tags handed out so far (tags start at 1) */
  uint64_t acked;     /* highest tag such that all tags <= it are settled */
  uint64_t nacked;
  uint64_t returned;
} confirm_state;

/* Records a confirmation for `tag`; if `multiple`, for every tag up to it. */
static void settle(confirm_state *st, uint64_t tag, int multiple, int nack) {
  uint64_t first = multiple ? st->acked + 1 : tag;
  uint64_t count = tag >= first ? tag - first + 1 : 0;

  if (nack) {
    st->nacked += count;
  }
  /* Individual (non-multiple) confirmations may arrive out of order; for this
   * simple batch scheme the broker confirms in order, so tracking the highest
   * settled tag is sufficient. A production publisher that needs per-message
   * outcomes keeps a map of outstanding tags to messages instead. */
  if (tag > st->acked) {
    st->acked = tag;
  }
}

/* Blocks until every published message has been confirmed. */
static int wait_for_confirms(amqp_connection_state_t conn, confirm_state *st) {
  struct timeval timeout = {CONFIRM_TIMEOUT_SECONDS, 0};

  while (st->acked < st->published) {
    amqp_frame_t frame;
    int status = amqp_simple_wait_frame_noblock(conn, &frame, &timeout);

    if (status == AMQP_STATUS_TIMEOUT) {
      fprintf(stderr, "Timed out: %" PRIu64 " message(s) never confirmed\n",
              st->published - st->acked);
      return TUTORIAL_ERROR;
    }
    if (tutorial_check_status(status, "Waiting for confirms") != TUTORIAL_OK) {
      return TUTORIAL_ERROR;
    }
    if (frame.frame_type != AMQP_FRAME_METHOD) {
      continue;
    }

    switch (frame.payload.method.id) {
      case AMQP_BASIC_ACK_METHOD: {
        amqp_basic_ack_t *ack =
            (amqp_basic_ack_t *)frame.payload.method.decoded;
        settle(st, ack->delivery_tag, ack->multiple, /*nack*/ 0);
        break;
      }
      case AMQP_BASIC_NACK_METHOD: {
        amqp_basic_nack_t *nack =
            (amqp_basic_nack_t *)frame.payload.method.decoded;
        settle(st, nack->delivery_tag, nack->multiple, /*nack*/ 1);
        break;
      }
      case AMQP_BASIC_RETURN_METHOD: {
        /* The returned message's header and body frames follow and must be
         * consumed to keep the connection in sync. */
        amqp_message_t returned;
        if (tutorial_check_reply(
                amqp_read_message(conn, frame.channel, &returned, 0),
                "Reading returned message") != TUTORIAL_OK) {
          return TUTORIAL_ERROR;
        }
        amqp_destroy_message(&returned);
        st->returned++;
        break;
      }
      case AMQP_CHANNEL_CLOSE_METHOD:
      case AMQP_CONNECTION_CLOSE_METHOD: {
        amqp_rpc_reply_t reply;
        reply.reply_type = AMQP_RESPONSE_SERVER_EXCEPTION;
        reply.reply = frame.payload.method;
        tutorial_check_reply(reply, "Publishing");
        return TUTORIAL_ERROR;
      }
      default:
        fprintf(stderr, "Unexpected method 0x%08X while waiting for confirms\n",
                (unsigned)frame.payload.method.id);
        return TUTORIAL_ERROR;
    }
  }
  return TUTORIAL_OK;
}

int main(int argc, char const *const *argv) {
  amqp_connection_state_t conn = NULL;
  confirm_state st;
  unsigned long count = DEFAULT_MESSAGE_COUNT;
  unsigned long i;
  int exit_code = EXIT_FAILURE;

  memset(&st, 0, sizeof(st));

  if (argc > 2) {
    char *end;
    errno = 0;
    count = strtoul(argv[2], &end, 10);
    if (errno != 0 || end == argv[2] || *end != '\0' || count == 0) {
      fprintf(stderr, "Usage: %s [amqp-url] [message-count]\n", argv[0]);
      return exit_code;
    }
  }

  if (tutorial_connect(tutorial_broker_url(argc, argv), &conn) != TUTORIAL_OK) {
    return exit_code;
  }

  amqp_queue_declare(conn, TUTORIAL_CHANNEL, amqp_cstring_bytes(QUEUE_NAME), 0,
                     /*durable*/ 1, 0, 0, amqp_empty_table);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Declaring queue") !=
      TUTORIAL_OK) {
    goto out;
  }

  /* Puts the channel in confirm mode. Irreversible for the channel's life. */
  amqp_confirm_select(conn, TUTORIAL_CHANNEL);
  if (tutorial_check_reply(amqp_get_rpc_reply(conn), "Enabling confirms") !=
      TUTORIAL_OK) {
    goto out;
  }

  for (i = 0; i < count; i++) {
    char body[32];
    amqp_basic_properties_t props;

    snprintf(body, sizeof(body), "message %lu", i + 1);
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = AMQP_DELIVERY_PERSISTENT;

    if (tutorial_check_status(
            amqp_basic_publish(conn, TUTORIAL_CHANNEL, amqp_empty_bytes,
                               amqp_cstring_bytes(QUEUE_NAME),
                               /*mandatory*/ 1, 0, &props,
                               amqp_cstring_bytes(body)),
            "Publishing") != TUTORIAL_OK) {
      goto out;
    }
    st.published++;

    if (st.published % BATCH_SIZE == 0 || i + 1 == count) {
      if (wait_for_confirms(conn, &st) != TUTORIAL_OK) {
        goto out;
      }
    }
  }

  printf(" [x] Published %" PRIu64 " messages: %" PRIu64 " nacked, %" PRIu64
         " returned as unroutable\n",
         st.published, st.nacked, st.returned);
  /* A nack or a return means at least one message was not safely delivered. */
  exit_code =
      (st.nacked == 0 && st.returned == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

out:
  tutorial_close(conn);
  return exit_code;
}
