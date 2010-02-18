#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <stdint.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <unistd.h>
#include <assert.h>

#include "example_utils.h"

#define SUMMARY_EVERY_US 1000000

static void run(amqp_connection_state_t conn)
{
  long long start_time = now_microseconds();
  int received = 0;
  int previous_received = 0;
  long long previous_report_time = start_time;
  long long next_summary_time = start_time + SUMMARY_EVERY_US;

  amqp_frame_t frame;
  int result;
  size_t body_received;
  size_t body_target;

  long long now;

  while (1) {
    now = now_microseconds();
    if (now > next_summary_time) {
      int countOverInterval = received - previous_received;
      double intervalRate = countOverInterval / ((now - previous_report_time) / 1000000.0);
      printf("%lld ms: Received %d - %d since last report (%d Hz)\n",
	     (now - start_time) / 1000, received, countOverInterval, (int) intervalRate);

      previous_received = received;
      previous_report_time = now;
      next_summary_time += SUMMARY_EVERY_US;
    }

    amqp_maybe_release_buffers(conn);
    result = amqp_simple_wait_frame(conn, &frame);
    if (result <= 0) return;

    if (frame.frame_type != AMQP_FRAME_METHOD)
      continue;

    if (frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD)
      continue;

    result = amqp_simple_wait_frame(conn, &frame);
    if (result <= 0) return;
    if (frame.frame_type != AMQP_FRAME_HEADER) {
      fprintf(stderr, "Expected header!");
      abort();
    }

    body_target = frame.payload.properties.body_size;
    body_received = 0;

    while (body_received < body_target) {
      result = amqp_simple_wait_frame(conn, &frame);
      if (result <= 0) return;

      if (frame.frame_type != AMQP_FRAME_BODY) {
	fprintf(stderr, "Expected body!");
	abort();
      }

      body_received += frame.payload.body_fragment.len;
      assert(body_received <= body_target);
    }

    received++;
  }
}

int main(int argc, char const * const *argv) {
  char const *hostname;
  int port;
  char const *exchange;
  char const *bindingkey;

  int sockfd;
  amqp_connection_state_t conn;

  amqp_bytes_t queuename;

  if (argc < 3) {
    fprintf(stderr, "Usage: amqp_consumer host port\n");
    return 1;
  }

  hostname = argv[1];
  port = atoi(argv[2]);
  exchange = "amq.direct"; //argv[3];
  bindingkey = "test queue"; //argv[4];

  conn = amqp_new_connection();

  die_on_error(sockfd = amqp_open_socket(hostname, port), "Opening socket");
  amqp_set_sockfd(conn, sockfd);
  die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
		    "Logging in");
  amqp_channel_open(conn, 1);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

  {
    amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1, AMQP_EMPTY_BYTES, 0, 0, 0, 1,
						    AMQP_EMPTY_TABLE);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
    queuename = amqp_bytes_malloc_dup(r->queue);
    if (queuename.bytes == NULL) {
      die_on_error(-ENOMEM, "Copying queue name");
    }
  }

  amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange), amqp_cstring_bytes(bindingkey),
		  AMQP_EMPTY_TABLE);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");

  amqp_basic_consume(conn, 1, queuename, AMQP_EMPTY_BYTES, 0, 1, 0, AMQP_EMPTY_TABLE);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

  run(conn);

  die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
  amqp_destroy_connection(conn);
  die_on_error(close(sockfd), "Closing socket");

  return 0;
}
