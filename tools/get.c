/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * The Original Code is librabbitmq.
 *
 * The Initial Developer of the Original Code is VMware, Inc.
 * Portions created by VMware are Copyright (c) 2007-2011 VMware, Inc.
 *
 * Portions created by Tony Garnock-Jones are Copyright (c) 2009-2010
 * VMware, Inc. and Tony Garnock-Jones.
 *
 * All rights reserved.
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

#include "common.h"

static int do_get(amqp_connection_state_t conn, char *queue)
{
	amqp_rpc_reply_t r
		= amqp_basic_get(conn, 1, cstring_bytes(queue), 1);
	die_rpc(r, "basic.get");
	
	if (r.reply.id == AMQP_BASIC_GET_EMPTY_METHOD)
		return 0;

	copy_body(conn, 1);
	return 1;
}

int main(int argc, const char **argv)
{
	amqp_connection_state_t conn;
	char *queue = NULL;
	int got_something;

	struct poptOption options[] = {
		INCLUDE_OPTIONS(connect_options),
		{"queue", 'q', POPT_ARG_STRING, &queue, 0,
		 "the queue to consume from", "queue"},
		POPT_AUTOHELP
		{ NULL, 0, 0, NULL, 0 }
	};
	
	process_all_options(argc, argv, options);

	if (!queue) {
		fprintf(stderr, "queue not specified\n");
		return 1;
	}
	
	conn = make_connection();
	got_something = do_get(conn, queue);
	close_connection(conn);
	return got_something ? 0 : 2;
}
