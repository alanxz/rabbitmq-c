/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
 *
 * Portions created by Alan Antonuk are Copyright (c) 2012-2013
 * Alan Antonuk. All Rights Reserved.
 *
 * Portions created by Mike Steinert are Copyright (c) 2012-2013
 * Mike Steinert. All Rights Reserved.
 *
 * Portions created by VMware are Copyright (c) 2007-2012 VMware, Inc.
 * All Rights Reserved.
 *
 * Portions created by Tony Garnock-Jones are Copyright (c) 2009-2010
 * VMware, Inc. and Tony Garnock-Jones. All Rights Reserved.
 *
 * Portions created by Adrien Pre are Copyright (c) 2017
 * Adrien Pre. All Rights Reserved.
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

/**
  \file
  \brief rabbitmq Tutorial 2 (worker)

Tutorial 2, worker program <i>rabbitmq-c</i> library available at
https://github.com/alanxz/rabbitmq-c
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include "assert.h"
#include "utils.h" // for amqp_dump(). todo: remove this dependency

int main(int argc, char const *const *argv)
{
    char const *hostname= "localhost";
    int port= 5672;
    int  status;

    char const *queueName= "task_queue";
    amqp_socket_t *socket = NULL;

    amqp_connection_state_t  conn;
    amqp_channel_t channel = 1;
    amqp_rpc_reply_t r;

    (void) argc; (void ) argv; // get rid of the unused variable warning



    conn = amqp_new_connection();       assert(conn!=NULL);
    socket = amqp_tcp_socket_new(conn); assert(socket!=NULL);

    status = amqp_socket_open(socket, hostname, port);
    if (status!=AMQP_STATUS_OK) {
        printf("failed to open TCP socket, is rabbitmq server running?\n");
        exit(1);
    }

    r=amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest");
    assert(r.reply_type==AMQP_RESPONSE_NORMAL);
    if (r.reply_type != AMQP_RESPONSE_NORMAL) {
        printf("Cannot login\n");
        exit(1);
    }

    amqp_channel_open(conn, channel);

    r = amqp_get_rpc_reply(conn);
    assert(r.reply_type==AMQP_RESPONSE_NORMAL);



    amqp_basic_consume(conn, 1, amqp_cstring_bytes(queueName), amqp_empty_bytes, 0, 0,0, amqp_empty_table);
    r = amqp_get_rpc_reply(conn);
    assert(r.reply_type==AMQP_RESPONSE_NORMAL);


    {
        for (;;) {
            amqp_rpc_reply_t res;
            amqp_envelope_t envelope;

            amqp_maybe_release_buffers(conn);

            res = amqp_consume_message(conn, &envelope, NULL, 0);

            if (AMQP_RESPONSE_NORMAL != res.reply_type) {
                break;
            }

            printf("Delivery %u, exchange %.*s routingkey %.*s\n",
                   (unsigned) envelope.delivery_tag,
                   (int) envelope.exchange.len, (char *) envelope.exchange.bytes,
                   (int) envelope.routing_key.len, (char *) envelope.routing_key.bytes);

            if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
                printf("Content-type: %.*s\n",
                       (int) envelope.message.properties.content_type.len,
                       (char *) envelope.message.properties.content_type.bytes);
            }
            printf("----\n");

            amqp_dump(envelope.message.body.bytes, envelope.message.body.len);

            int ack_res = amqp_basic_ack (conn,channel,envelope.delivery_tag,0);

            if (ack_res != 0) {
                printf("failed to ack work\n");
                exit(1);
            }
            assert (ack_res == 0);
            amqp_destroy_envelope(&envelope);
        }
    }

    r=amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);

    r=amqp_connection_close(conn, AMQP_REPLY_SUCCESS); assert(r.reply_type==AMQP_RESPONSE_NORMAL);
    status=amqp_destroy_connection(conn);   assert (status==AMQP_STATUS_OK);
    return 0;
}


