// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

#include "rabbitmq-c/framing.h"
#include "utils.h"

#define SUMMARY_EVERY_US 5000

static void send_batch(amqp_connection_state_t conn, char const *queue_name,
                       int rate_limit, int message_count) {
  uint64_t start_time = now_microseconds();
  int i;
  int sent = 0;
  int previous_sent = 0;
  uint64_t previous_report_time = start_time;
  uint64_t next_summary_time = start_time + SUMMARY_EVERY_US;

  char message[256];
  amqp_bytes_t message_bytes;

  for (i = 0; i < (int)sizeof(message); i++) {
    message[i] = i & 0xff;
  }

  message_bytes.len = sizeof(message);
  message_bytes.bytes = message;

  for (i = 0; i < message_count; i++) {
    uint64_t now = now_microseconds();

    die_on_error(amqp_basic_publish(conn, 1, amqp_cstring_bytes("amq.direct"),
                                    amqp_cstring_bytes(queue_name), 0, 0, NULL,
                                    message_bytes),
                 "Publishing");
    sent++;
    if (now > next_summary_time) {
      int countOverInterval = sent - previous_sent;
      double intervalRate =
          countOverInterval / ((now - previous_report_time) / 1000000.0);
      printf("%d ms: Sent %d - %d since last report (%d Hz)\n",
             (int)(now - start_time) / 1000, sent, countOverInterval,
             (int)intervalRate);

      previous_sent = sent;
      previous_report_time = now;
      next_summary_time += SUMMARY_EVERY_US;
    }

    while (((i * 1000000.0) / (now - start_time)) > rate_limit) {
      microsleep(2000);
      now = now_microseconds();
    }
  }

  {
    uint64_t stop_time = now_microseconds();
    int total_delta = (int)(stop_time - start_time);

    printf("PRODUCER - Message count: %d\n", message_count);
    printf("Total time, milliseconds: %d\n", total_delta / 1000);
    printf("Overall messages-per-second: %g\n",
           (message_count / (total_delta / 1000000.0)));
  }
}

#define CONSUME_TIMEOUT_USEC 100
#define WAITING_TIMEOUT_USEC (30 * 1000)
void wait_for_acks(amqp_connection_state_t conn) {
  uint64_t start_time = now_microseconds();
  amqp_basic_ack_t ack;
  struct timeval timeout = {0, CONSUME_TIMEOUT_USEC};
  uint64_t now;
  amqp_envelope_t envelope;

  for (;;) {
    amqp_rpc_reply_t ret;

    now = now_microseconds();

    if (now > start_time + WAITING_TIMEOUT_USEC) {
      return;
    }

    amqp_maybe_release_buffers(conn);
    ret = amqp_publisher_confirm_wait(conn, &envelope, &ack, &timeout);

    if (AMQP_RESPONSE_LIBRARY_EXCEPTION == ret.reply_type) {
      if (AMQP_STATUS_UNEXPECTED_STATE == ret.library_error) {
        fprintf(stderr, "An unexpected method was received\n");
        return;
      } else {
        die_on_amqp_error(ret, "Waiting for publisher confirmation");
      }
    }

    fprintf(stderr, "Got an ACK!\n");
    fprintf(stderr, "Here's the ACK:\n");
    fprintf(stderr, "\tdelivery_tag: «%" PRIu64 "»\n", ack.delivery_tag);
    fprintf(stderr, "\tmultiple: «%d»\n", ack.multiple);
  }
}

int main(int argc, char const *const *argv) {
  char const *hostname;
  int port, status;
  int rate_limit;
  int message_count;
  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;

  if (argc < 5) {
    fprintf(stderr,
            "Usage: amqp_producer host port rate_limit message_count\n");
    return 1;
  }

  hostname = argv[1];
  port = atoi(argv[2]);
  rate_limit = atoi(argv[3]);
  message_count = atoi(argv[4]);

  conn = amqp_new_connection();

  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    die("creating TCP socket");
  }

  status = amqp_socket_open(socket, hostname, port);
  if (status) {
    die("opening TCP socket");
  }

  die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                               "guest", "guest"),
                    "Logging in");
  amqp_channel_open(conn, 1);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

  // Enable confirm_select
  amqp_confirm_select(conn, 1);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Enable confirm-select");

  send_batch(conn, "test queue", rate_limit, message_count);

  wait_for_acks(conn);
  die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
                    "Closing channel");
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
                    "Closing connection");
  die_on_error(amqp_destroy_connection(conn), "Ending connection");
  return 0;
}
