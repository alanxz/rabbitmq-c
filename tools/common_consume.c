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
#include <stdlib.h>

#include <popt.h>

#include "common.h"

static char *queue;
static char *exchange;
static char *exchange_type;
static char *routing_key;

const char *consume_queue_options_title = "Source queue options";
struct poptOption consume_queue_options[] = {
	{"queue", 'q', POPT_ARG_STRING, &queue, 0,
	 "the queue to consume from", "queue"},
	{"exchange", 'e', POPT_ARG_STRING, &exchange, 0,
	 "bind the queue to this exchange", "exchange"},
	{"exchange-type", 't', POPT_ARG_STRING, &exchange_type, 0,
	 "create auto-delete exchange of this type for binding", "type"},
	{"routing-key", 'r', POPT_ARG_STRING, &routing_key, 0,
	 "the routing key to bind with", "routing key"},
	{ NULL, 0, 0, NULL, 0 }
};

/* Convert a amqp_bytes_t to an escaped string form for printing.  We
   use the same escaping conventions as rabbitmqctl. */
static char *stringify_bytes(amqp_bytes_t bytes)
{
	/* W will need up to 4 chars per byte, plus the terminating 0 */
	char *res = malloc(bytes.len * 4 + 1);
	uint8_t *data = bytes.bytes;
	char *p = res;
	size_t i;
	
	for (i = 0; i < bytes.len; i++) {
		if (data[i] >= 32 && data[i] != 127) {
			*p++ = data[i];
		}
		else {
			*p++ = '\\';
			*p++ = '0' + (data[i] >> 6);
			*p++ = '0' + (data[i] >> 3 & 0x7); 
			*p++ = '0' + (data[i] & 0x7);
		}
	}

	*p = 0;
	return res;
}

amqp_bytes_t setup_queue(amqp_connection_state_t conn)
{
	/* if an exchange name wasn't provided, check that we don't
	   have options that require it. */
	if (!exchange) {
		char *opt = NULL;
		if (routing_key)
			opt = "--routing-key";
		else if (exchange_type)
			opt = "--exchange-type";

		if (opt) {
			fprintf(stderr,
				"%s option requires an exchange name to be "
				"provided with --exchange\n", opt);
			exit(1);
		}
	}

	/* Declare the queue.  If the queue already exists, this won't have
	   any effect. */
	amqp_bytes_t queue_bytes = cstring_bytes(queue);
	amqp_queue_declare_ok_t *res
		= amqp_queue_declare(conn, 1, queue_bytes, 0, 0, 0, 1,
				     AMQP_EMPTY_TABLE);
	if (!res)
		die_rpc(amqp_get_rpc_reply(conn), "queue.declare");

	if (!queue) {
		// the server should have provided a queue name
		char *sq;
		queue_bytes = amqp_bytes_malloc_dup(res->queue);
		sq = stringify_bytes(queue_bytes);
		fprintf(stderr, "Server provided queue name: %s\n", sq);
		free(sq);
	}

	/* Bind to an exchange if requested */
	if (exchange) {
		amqp_bytes_t eb = amqp_cstring_bytes(exchange);
		
		if (exchange_type) {
			// we should create the exchange
			if (!amqp_exchange_declare(conn, 1, eb, 
					     amqp_cstring_bytes(exchange_type),
					     0, 0, 1, AMQP_EMPTY_TABLE))
			die_rpc(amqp_get_rpc_reply(conn), "exchange.declare");
		}
		
		if (!amqp_queue_bind(conn, 1, queue_bytes, eb,
				     cstring_bytes(routing_key),
				     AMQP_EMPTY_TABLE))
			die_rpc(amqp_get_rpc_reply(conn), "queue.bind");
	}
	
	return queue_bytes;
}
