#include "config.h"

#include <stdio.h>

#include <popt.h>

#include "common.h"
#include "common_consume.h"

static int do_get(amqp_connection_state_t conn)
{
	amqp_rpc_reply_t r
		= amqp_basic_get(conn, 1, setup_queue(conn), 1);
	die_rpc(r, "basic.get");
	
	if (r.reply.id == AMQP_BASIC_GET_EMPTY_METHOD)
		return 0;

	copy_body(conn, 1);
	return 1;
}

int main(int argc, const char **argv)
{
	amqp_connection_state_t conn;
	int got_something;

	struct poptOption options[] = {
		INCLUDE_OPTIONS(connect_options),
		INCLUDE_OPTIONS(consume_queue_options),
		POPT_AUTOHELP
		{ NULL, 0, 0, NULL, 0 }
	};
	
	process_all_options(argc, argv, options);
	
	conn = make_connection();
	got_something = do_get(conn);
	close_connection(conn);
	return got_something ? 0 : 2;
}
