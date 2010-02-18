#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <unistd.h>

#include "example_utils.h"

#define SUMMARY_EVERY_US 1000000

static void send_batch(amqp_connection_state_t conn,
		       char const *queue_name,
		       int rate_limit,
		       int message_count)
{
  long long start_time = now_microseconds();
  int i;
  int sent = 0;
  int previous_sent = 0;
  long long previous_report_time = start_time;
  long long next_summary_time = start_time + SUMMARY_EVERY_US;

  char message[256];

  for (i = 0; i < sizeof(message); i++) {
    message[i] = i & 0xff;
  }

  for (i = 0; i < message_count; i++) {
    long long now = now_microseconds();
    die_on_error(amqp_basic_publish(conn,
				    1,
				    amqp_cstring_bytes("amq.direct"),
				    amqp_cstring_bytes(queue_name),
				    0,
				    0,
				    NULL,
				    (amqp_bytes_t) {.len = sizeof(message), .bytes = message}),
		 "Publishing");
    sent++;
    if (now > next_summary_time) {
      int countOverInterval = sent - previous_sent;
      double intervalRate = countOverInterval / ((now - previous_report_time) / 1000000.0);
      printf("%lld ms: Sent %d - %d since last report (%d Hz)\n",
	     (now - start_time) / 1000, sent, countOverInterval, (int) intervalRate);

      previous_sent = sent;
      previous_report_time = now;
      next_summary_time += SUMMARY_EVERY_US;
    }

    while (((i * 1000000.0) / (now - start_time)) > rate_limit) {
      usleep(2000);
      now = now_microseconds();
    }
  }

  {
    long long stop_time = now_microseconds();
    long long total_delta = stop_time - start_time;

    printf("PRODUCER - Message count: %d\n", message_count);
    printf("Total time, milliseconds: %lld\n", total_delta / 1000);
    printf("Overall messages-per-second: %g\n", (message_count / (total_delta / 1000000.0)));
  }
}

int main(int argc, char const * const *argv) {
  char const *hostname;
  int port;
  int rate_limit;
  int message_count;

  int sockfd;
  amqp_connection_state_t conn;

  if (argc < 5) {
    fprintf(stderr, "Usage: amqp_producer host port rate_limit message_count\n");
    return 1;
  }

  hostname = argv[1];
  port = atoi(argv[2]);
  rate_limit = atoi(argv[3]);
  message_count = atoi(argv[4]);

  conn = amqp_new_connection();

  die_on_error(sockfd = amqp_open_socket(hostname, port), "Opening socket");
  amqp_set_sockfd(conn, sockfd);
  die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
		    "Logging in");
  amqp_channel_open(conn, 1);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

  send_batch(conn, "test queue", rate_limit, message_count);

  die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
  amqp_destroy_connection(conn);
  die_on_error(close(sockfd), "Closing socket");
  return 0;
}
