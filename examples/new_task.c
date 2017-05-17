
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
  \brief rabbitmq Tutorial 2 (new_task)
*/

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
    char const *messagebody= "Hello World!";
    char const *exchange = "";
    amqp_socket_t *socket = NULL;

    amqp_connection_state_t conn;
    //    amqp_bytes_t exchange_param;
    //   amqp_bytes_t channel_type_param;
    //   amqp_table_t arguments;
    amqp_bytes_t queuename;
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

    amqp_channel_open(conn, 1);

    r = amqp_get_rpc_reply(conn);
    assert(r.reply_type==AMQP_RESPONSE_NORMAL);

    {
        amqp_queue_declare_ok_t *rq = amqp_queue_declare(conn,
                                                         1,
                                                         amqp_cstring_bytes(queueName),
                                                         //  amqp_empty_bytes,
                                                         0,
                                                         1,// durable
                                                         0,0,
                                                         amqp_empty_table);

        r = amqp_get_rpc_reply(conn);
        assert(r.reply_type==AMQP_RESPONSE_NORMAL);

        queuename = amqp_bytes_malloc_dup(rq->queue);
        if (queuename.bytes == NULL) {
            fprintf(stderr, "Out of memory while copying queue name");
            return 1;
        }
    }


    {
        amqp_basic_properties_t props;
        props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
        props.content_type = amqp_cstring_bytes("text/plain");
        props.delivery_mode = AMQP_DELIVERY_PERSISTENT; /* persistent delivery mode */
        status = amqp_basic_publish(conn,
                                    1,
                                    amqp_cstring_bytes(exchange),
                                    amqp_cstring_bytes(queueName),
                                    0,
                                    0,
                                    &props,
                                    amqp_cstring_bytes(messagebody));
        assert(status==AMQP_STATUS_OK);
    }

    r=amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);

    r=amqp_connection_close(conn, AMQP_REPLY_SUCCESS); assert(r.reply_type==AMQP_RESPONSE_NORMAL);
    status=amqp_destroy_connection(conn);   assert (status==AMQP_STATUS_OK);
    return 0;
}
