#include "config.h"

#include <stdio.h>

#include <popt.h>

#include "common.h"
#include "common_consume.h"

static void do_consume(amqp_connection_state_t conn, int no_ack,
		       const char * const *argv)
{
	if (!amqp_basic_consume(conn, 1, setup_queue(conn),
			       AMQP_EMPTY_BYTES, 0, no_ack, 0))
		die_rpc(amqp_get_rpc_reply(conn), "basic.consume");

	for (;;) {
		amqp_frame_t frame;
		struct pipeline pl;
		uint64_t delivery_tag;
		int res = amqp_simple_wait_frame(conn, &frame);
		if (res < 0)
			die_errno(-res, "waiting for header frame");

		if (frame.frame_type != AMQP_FRAME_METHOD
		    || frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD)
			continue;

		amqp_basic_deliver_t *deliver
			= (amqp_basic_deliver_t *)frame.payload.method.decoded;
		delivery_tag = deliver->delivery_tag;
		
		pipeline(argv, &pl);
		copy_body(conn, pl.infd);

		if (finish_pipeline(&pl) && !no_ack)
			die_errno(-amqp_basic_ack(conn, 1, delivery_tag, 0),
				  "basic.ack");

		amqp_maybe_release_buffers(conn);
	}
}

int main(int argc, const char **argv)
{
	poptContext opts;
	int no_ack;
	amqp_connection_state_t conn;
	const char * const *cmd_argv;
	
	struct poptOption options[] = {
		INCLUDE_OPTIONS(connect_options),
		INCLUDE_OPTIONS(consume_queue_options),
		{"no-ack", 'A', POPT_ARG_NONE, &no_ack, 0,
		 "consume in no-ack mode", NULL},
		POPT_AUTOHELP
		{ NULL, 0, 0, NULL, 0 }
	};
	
	opts = process_options(argc, argv, options,
			       "[OPTIONS]... <command> <args>");
	
	cmd_argv = poptGetArgs(opts);
	if (!cmd_argv[0]) {
		fprintf(stderr, "consuming command not specified");
		goto error;
	}

	conn = make_connection();
	do_consume(conn, no_ack, cmd_argv);
	close_connection(conn);
	return 0;

error:
	poptFreeContext(opts);
	return 1;
}
