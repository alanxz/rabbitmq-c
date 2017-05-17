/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
/*
 ***** BEGIN LICENSE BLOCK *****
           DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                   Version 2, December 2004

Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

Everyone is permitted to copy and distribute verbatim or modified
copies of this license document, and changing it is allowed as long
as the name is changed.

           DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

 0. You just DO WHAT THE FUCK YOU WANT TO.

***** END LICENSE BLOCK *****
 */

/**
  \file
  \brief rabbitmq Tutorial 2 (worker)
  
Tutorial 1 receive using <i>rabbitmq-c</i> library available at
https://github.com/alanxz/rabbitmq-c
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>


#if defined(NDEBUG)
#undef NDEBUG
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include "assert.h"



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


    conn = amqp_new_connection();       assert(conn!=NULL);
    socket = amqp_tcp_socket_new(conn); assert(socket!=NULL);

    status = amqp_socket_open(socket, hostname, port);
    if (status!=AMQP_STATUS_OK) {
        printf("failed to open TCP socket, is rabbitmq server running?\n");
        exit(1);
    }

    r=amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest");
    assert(r.reply_type==AMQP_RESPONSE_NORMAL);

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
         /*
         int amqp_basic_ack	(	amqp_connection_state_t 	state,
         amqp_channel_t 	channel,
         uint64_t 	delivery_tag,
         amqp_boolean_t 	multiple
         )	*/
         int ack_res = amqp_basic_ack (conn,channel,envelope.delivery_tag,0);
         assert (ack_res == 0);
         amqp_destroy_envelope(&envelope);
       }
     }




    r=amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);

    r=amqp_connection_close(conn, AMQP_REPLY_SUCCESS); assert(r.reply_type==AMQP_RESPONSE_NORMAL);
    status=amqp_destroy_connection(conn);   assert (status==AMQP_STATUS_OK);
    return 0;
}


