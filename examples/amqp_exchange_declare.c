#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <unistd.h>

#include "example_utils.h"

int main(int argc, char const * const *argv) {
  char const *hostname;
  int port;
  char const *exchange;
  char const *exchangetype;

  int sockfd;
  amqp_connection_state_t conn;

  if (argc < 5) {
    fprintf(stderr, "Usage: amqp_exchange_declare host port exchange exchangetype\n");
    return 1;
  }

  hostname = argv[1];
  port = atoi(argv[2]);
  exchange = argv[3];
  exchangetype = argv[4];

  conn = amqp_new_connection();

  die_on_error(sockfd = amqp_open_socket(hostname, port), "Opening socket");
  amqp_set_sockfd(conn, sockfd);
  die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
		    "Logging in");
  amqp_channel_open(conn, 1);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

  amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes(exchangetype),
			0, 0, 0, AMQP_EMPTY_TABLE);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring exchange");

  die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
  amqp_destroy_connection(conn);
  die_on_error(close(sockfd), "Closing socket");
  return 0;
}
