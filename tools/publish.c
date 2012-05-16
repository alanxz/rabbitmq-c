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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

static void do_publish(amqp_connection_state_t conn,
                       char *exchange, char *routing_key,
		       amqp_basic_properties_t *props, amqp_bytes_t body)
{
	int res = amqp_basic_publish(conn, 1,
				     cstring_bytes(exchange),
				     cstring_bytes(routing_key),
				     0, 0, props, body);
	die_amqp_error(res, "basic.publish");
}

int main(int argc, const char **argv)
{
	amqp_connection_state_t conn;
	char *exchange = NULL;
	char *routing_key = NULL;
	char *content_type = NULL;
	char *content_encoding = NULL;
	char *body = NULL;
	amqp_basic_properties_t props;
	amqp_bytes_t body_bytes;
	int delivery = 1; /* non-persistent by default */

	struct poptOption options[] = {
		INCLUDE_OPTIONS(connect_options),
		{"exchange", 'e', POPT_ARG_STRING, &exchange, 0,
		 "the exchange to publish to", "exchange"},
		{"routing-key", 'r', POPT_ARG_STRING, &routing_key, 0,
		 "the routing key to publish with", "routing key"},
		{"persistent", 'p', POPT_ARG_VAL, &delivery, 2,
		 "use the persistent delivery mode", NULL},
		{"content-type", 'C', POPT_ARG_STRING, &content_type, 0,
		 "the content-type for the message", "content type"},
		{"content-encoding", 'E', POPT_ARG_STRING,
		 &content_encoding, 0,
		 "the content-encoding for the message", "content encoding"},
		{"body", 'b', POPT_ARG_STRING, &body, 0,
                 "specify the message body", "body"},
		POPT_AUTOHELP
		{ NULL, '\0', 0, NULL, 0, NULL, NULL }
	};

	process_all_options(argc, argv, options);

	if (!exchange && !routing_key) {
		fprintf(stderr,
			"neither exchange nor routing key specified\n");
		return 1;
	}

	memset(&props, 0, sizeof props);
	props._flags = AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.delivery_mode = 2; /* persistent delivery mode */

	if (content_type) {
		props._flags |= AMQP_BASIC_CONTENT_TYPE_FLAG;
		props.content_type = amqp_cstring_bytes(content_type);
	}

	if (content_encoding) {
		props._flags |= AMQP_BASIC_CONTENT_ENCODING_FLAG;
		props.content_encoding = amqp_cstring_bytes(content_encoding);
	}

	conn = make_connection();

	if (body)
		body_bytes = amqp_cstring_bytes(body);
	else
		body_bytes = read_all(0);

	do_publish(conn, exchange, routing_key, &props, body_bytes);

	if (!body)
		free(body_bytes.bytes);

	close_connection(conn);
	return 0;
}
