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

#include <stdint.h>

#include <amqp.h>
#include <amqp_framing.h>

extern char *amqp_server_exception_string(amqp_rpc_reply_t r);
extern char *amqp_rpc_reply_string(amqp_rpc_reply_t r);

extern void die(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));
extern void die_errno(int err, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));
extern void die_rpc(amqp_rpc_reply_t r, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

extern const char *connect_options_title;
extern struct poptOption connect_options[];
extern amqp_connection_state_t make_connection(void);
extern void close_connection(amqp_connection_state_t conn);

extern amqp_bytes_t read_all(int fd);
extern void write_all(int fd, amqp_bytes_t data);

extern void copy_body(amqp_connection_state_t conn, int fd);

struct pipeline {
	int pid;
	int infd;
};

extern void pipeline(const char * const *argv, struct pipeline *pl);
extern int finish_pipeline(struct pipeline *pl);

#define INCLUDE_OPTIONS(options) \
	{NULL, 0, POPT_ARG_INCLUDE_TABLE, options, 0, options ## _title, NULL}

extern poptContext process_options(int argc, const char **argv,
				   struct poptOption *options,
				   const char *help);
extern void process_all_options(int argc, const char **argv,
				struct poptOption *options);

extern amqp_bytes_t cstring_bytes(const char *str);
