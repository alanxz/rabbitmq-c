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

#include <assert.h>

#include "utils.h"

int main(int argc, char* argv[]) {

  int sockfd;
  amqp_connection_state_t conn;

  if (argc < 6) { // minimum number of mandatory arguments
    fprintf(stderr, "usage:\namqp_rpc_sendstring_client -h hostname [-p port] [-vh virtualhost] [-e exchange] -r routingkey messagebody\n");
    return 1;
  }

  const char* const hostname    = getopt_str("-h", argc, argv, "");
  const int         port        = getopt_int("-p", argc, argv, 5672);
  const char* const virtualhost = getopt_str("-vh", argc, argv, "/");
  const char* const exchange    = getopt_str("-e", argc, argv, "");
  const char* const routingkey  = getopt_str("-r", argc, argv, "");
  const char* const messagebody = argv[argc - 1];

  /*
     establish a channel that is used to connect RabbitMQ server
  */

  conn = amqp_new_connection();

  die_on_error(sockfd = amqp_open_socket(hostname, port), "Opening socket");
  amqp_set_sockfd(conn, sockfd);
  die_on_amqp_error(amqp_login(conn, virtualhost, 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
                    "Logging in");
  amqp_channel_open(conn, 1);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

  /*
     create private reply_to queue
  */

  amqp_bytes_t reply_to_queue;
  {
    amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
    reply_to_queue = amqp_bytes_malloc_dup(r->queue);
    if (reply_to_queue.bytes == NULL) {
      fprintf(stderr, "Out of memory while copying queue name");
      return 1;
    }
  }

  /*
     send the message
  */

  {
    /*
      set properties
    */
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG |
                   AMQP_BASIC_DELIVERY_MODE_FLAG |
                   AMQP_BASIC_REPLY_TO_FLAG |
                   AMQP_BASIC_CORRELATION_ID_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; /* persistent delivery mode */
    props.reply_to = amqp_bytes_malloc_dup(reply_to_queue); /* is this a memory leak ? */
    if (props.reply_to.bytes == NULL) {
      fprintf(stderr, "Out of memory while copying queue name");
      return 1;
    }
    props.correlation_id = amqp_cstring_bytes("1");

    /*
      publish
    */
    die_on_error(amqp_basic_publish(conn,
                                    1,
                                    amqp_cstring_bytes(exchange),
                                    amqp_cstring_bytes(routingkey),
                                    0,
                                    0,
                                    &props,
                                    amqp_cstring_bytes(messagebody)),
                 "Publishing");
  }

  /*
    wait an answer
  */

  {
    amqp_basic_consume(conn, 1, reply_to_queue, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

    {
      amqp_frame_t frame;
      int result;

      amqp_basic_deliver_t *d;
      amqp_basic_properties_t *p;
      size_t body_target;
      size_t body_received;

      while (1) {
        amqp_maybe_release_buffers(conn);
        result = amqp_simple_wait_frame(conn, &frame);
        printf("Result %d\n", result);
        if (result < 0)
          break;

        printf("Frame type %d, channel %d\n", frame.frame_type, frame.channel);
        if (frame.frame_type != AMQP_FRAME_METHOD)
          continue;

        printf("Method %s\n", amqp_method_name(frame.payload.method.id));
        if (frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD)
          continue;

        d = (amqp_basic_deliver_t *) frame.payload.method.decoded;
        printf("Delivery %u, exchange %.*s routingkey %.*s\n",
               (unsigned) d->delivery_tag,
               (int) d->exchange.len, (char *) d->exchange.bytes,
               (int) d->routing_key.len, (char *) d->routing_key.bytes);

        result = amqp_simple_wait_frame(conn, &frame);
        if (result < 0)
          break;

        if (frame.frame_type != AMQP_FRAME_HEADER) {
          fprintf(stderr, "Expected header!");
          abort();
        }
        p = (amqp_basic_properties_t *) frame.payload.properties.decoded;
        if (p->_flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
          printf("Content-type: %.*s\n",
                 (int) p->content_type.len, (char *) p->content_type.bytes);
        }
        printf("----\n");

        body_target = frame.payload.properties.body_size;
        body_received = 0;

        while (body_received < body_target) {
          result = amqp_simple_wait_frame(conn, &frame);
          if (result < 0)
            break;

          if (frame.frame_type != AMQP_FRAME_BODY) {
            fprintf(stderr, "Expected body!");
            abort();
          }

          body_received += frame.payload.body_fragment.len;
          assert(body_received <= body_target);

          amqp_dump(frame.payload.body_fragment.bytes,
                    frame.payload.body_fragment.len);
        }

        if (body_received != body_target) {
          /* Can only happen when amqp_simple_wait_frame returns <= 0 */
          /* We break here to close the connection */
          break;
        }

        /* everything was fine, we can quit now because we received the reply */
        break;
      }

    }
  }

  /*
     closing
  */

  die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
  die_on_error(amqp_destroy_connection(conn), "Ending connection");

  return 0;
}