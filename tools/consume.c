/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * The Original Code is librabbitmq.
 *
 * The Initial Developers of the Original Code are LShift Ltd, Cohesive
 * Financial Technologies LLC, and Rabbit Technologies Ltd.  Portions
 * created before 22-Nov-2008 00:00:00 GMT by LShift Ltd, Cohesive
 * Financial Technologies LLC, or Rabbit Technologies Ltd are Copyright
 * (C) 2007-2008 LShift Ltd, Cohesive Financial Technologies LLC, and
 * Rabbit Technologies Ltd.
 *
 * Portions created by LShift Ltd are Copyright (C) 2007-2009 LShift
 * Ltd. Portions created by Cohesive Financial Technologies LLC are
 * Copyright (C) 2007-2009 Cohesive Financial Technologies
 * LLC. Portions created by Rabbit Technologies Ltd are Copyright (C)
 * 2007-2009 Rabbit Technologies Ltd.
 *
 * Portions created by Tony Garnock-Jones are Copyright (C) 2009-2010
 * LShift Ltd and Tony Garnock-Jones.
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2 or later (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GPL, and not to allow others to use your
 * version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the
 * notice and other provisions required by the GPL. If you do not
 * delete the provisions above, a recipient may use your version of
 * this file under the terms of any one of the MPL or the GPL.
 *
 * ***** END LICENSE BLOCK *****
 */

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
	if (!cmd_argv || !cmd_argv[0]) {
		fprintf(stderr, "consuming command not specified\n");
		poptPrintUsage(opts, stderr, 0);
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
