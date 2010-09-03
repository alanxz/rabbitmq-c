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
		{ NULL, 0, 0, NULL, 0 }
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
