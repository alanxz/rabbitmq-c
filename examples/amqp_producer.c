/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
 *
 * Portions created by VMware are Copyright (c) 2007-2012 VMware, Inc.
 * All Rights Reserved.
 *
 * Portions created by Tony Garnock-Jones are Copyright (c) 2009-2010
 * VMware, Inc. and Tony Garnock-Jones. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ***** END LICENSE BLOCK *****
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp.h>
#include <amqp_framing.h>

#include "utils.h"

#define SUMMARY_EVERY_US 1000000

static void send_batch(amqp_connection_state_t conn,
                       char const *exchange,
		       char const *routingkey,
		       int rate_limit,
		       int message_count)
{
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

    die_on_error(amqp_basic_publish(conn,
				    1,
				    amqp_cstring_bytes(exchange),
				    amqp_cstring_bytes(routingkey),
				    0,
				    0,
				    NULL,
				    message_bytes),
		 "Publishing");
    sent++;
    if (now > next_summary_time) {
      int countOverInterval = sent - previous_sent;
      double intervalRate = countOverInterval / ((now - previous_report_time) / 1000000.0);
      printf("%d ms: Sent %d - %d since last report (%d Hz)\n",
	     (int)(now - start_time) / 1000, sent, countOverInterval, (int) intervalRate);

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
    int total_delta = stop_time - start_time;

    printf("PRODUCER - Message count: %d\n", message_count);
    printf("Total time, milliseconds: %d\n", total_delta / 1000);
    printf("Overall messages-per-second: %g\n", (message_count / (total_delta / 1000000.0)));
  }
}

int main(int argc, char *argv[]) {
  char const *hostname;
  int port;
  char const *virtualhost;
  char const *exchange;
  char const *routingkey;
  int rate_limit = 2;
  int message_count = 10;

  int sockfd;
  amqp_connection_state_t conn;

  /*
    command line options
  */

  hostname    = getopt_str("-h", argc, argv, "");
  port        = getopt_int("-p", argc, argv, 5672);
  virtualhost = getopt_str("-vh", argc, argv, "/");
  exchange    = getopt_str("-e", argc, argv, "amq.direct");
  routingkey  = getopt_str("-r", argc, argv, "test_queue");
  if (argc > 4) {
    rate_limit  = strtol(argv[argc-2], NULL, 10);
    message_count = strtol(argv[argc-1], NULL, 10);
  }

  printf("hostname = %s\nport = %d\nvirtualhost = %s\nexchange = %s\nroutingkey = %s\nrate_limit = %d\nmessage_count = %d\n----\n",
         hostname, port, virtualhost, exchange, routingkey, rate_limit, message_count);

  if (strlen(hostname) == 0) {
    fprintf(stderr, "usage:\namqp_producer -h hostname [-p port] [-vh virtualhost] [-e exchange] [-r routingkey] rate_limit message_count\n");
    return 1;
  }

  conn = amqp_new_connection();

  die_on_error(sockfd = amqp_open_socket(hostname, port), "Opening socket");
  amqp_set_sockfd(conn, sockfd);
  die_on_amqp_error(amqp_login(conn, virtualhost, 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
		    "Logging in");
  amqp_channel_open(conn, 1);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

  send_batch(conn, exchange, routingkey, rate_limit, message_count);

  die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
  die_on_error(amqp_destroy_connection(conn), "Ending connection");
  return 0;
}
