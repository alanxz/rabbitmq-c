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
#include <stdarg.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "common.h"

#ifdef WINDOWS
#include "compat.h"
#endif

void die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(1);
}

void die_errno(int err, const char *fmt, ...)
{
	va_list ap;

	if (err == 0)
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, ": %s\n", strerror(errno));
	exit(1);
}

void die_amqp_error(int err, const char *fmt, ...)
{
	va_list ap;
	char *errstr;

	if (err >= 0)
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, ": %s\n", errstr = amqp_error_string(-err));
	free(errstr);
	exit(1);
}

char *amqp_server_exception_string(amqp_rpc_reply_t r)
{
	int res;
	char *s;

	switch (r.reply.id) {
	case AMQP_CONNECTION_CLOSE_METHOD: {
		amqp_connection_close_t *m
			= (amqp_connection_close_t *)r.reply.decoded;
		res = asprintf(&s, "server connection error %d, message: %.*s",
			       m->reply_code,
			       (int)m->reply_text.len,
			       (char *)m->reply_text.bytes);
		break;
	}

	case AMQP_CHANNEL_CLOSE_METHOD: {
		amqp_channel_close_t *m
			= (amqp_channel_close_t *)r.reply.decoded;
		res = asprintf(&s, "server channel error %d, message: %.*s",
			       m->reply_code,
			       (int)m->reply_text.len,
			       (char *)m->reply_text.bytes);
		break;
	}

	default:
		res = asprintf(&s, "unknown server error, method id 0x%08X",
			       r.reply.id);
		break;
	}

	return res >= 0 ? s : NULL;
}

char *amqp_rpc_reply_string(amqp_rpc_reply_t r)
{
	switch (r.reply_type) {
	case AMQP_RESPONSE_NORMAL:
		return strdup("normal response");
		
	case AMQP_RESPONSE_NONE:
		return strdup("missing RPC reply type");

	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		return amqp_error_string(r.library_error);

	case AMQP_RESPONSE_SERVER_EXCEPTION:
		return amqp_server_exception_string(r);

	default:
		abort();
	}
}

void die_rpc(amqp_rpc_reply_t r, const char *fmt, ...)
{
	va_list ap;
	char *errstr;

	if (r.reply_type == AMQP_RESPONSE_NORMAL)
		return;
	
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, ": %s\n", errstr = amqp_rpc_reply_string(r));
	free(errstr);
	exit(1);
}

static char *amqp_server = "localhost";
static char *amqp_vhost = "/";
static char *amqp_username = "guest";
static char *amqp_password = "guest";

const char *connect_options_title = "Connection options";
struct poptOption connect_options[] = {
	{"server", 's', POPT_ARG_STRING, &amqp_server, 0,
	 "the AMQP server to connect to", "hostname:port"},
	{"vhost", 0, POPT_ARG_STRING, &amqp_vhost, 0,
	 "the vhost to use when connecting", "vhost"},
	{"username", 0, POPT_ARG_STRING, &amqp_username, 0,
	 "the username to login with", "username"},
	{"password", 0, POPT_ARG_STRING, &amqp_password, 0,
	 "the password to login with", "password"},
	{ NULL, 0, 0, NULL, 0 }
};

amqp_connection_state_t make_connection(void)
{
	int s;
	amqp_connection_state_t conn;
	char *host = amqp_server;
	int port = 0;
	
	/* parse the server string into a hostname and a port */
	char *colon = strchr(amqp_server, ':');
	if (colon) {
		char *port_end;
		size_t host_len = colon - amqp_server;
		host = malloc(host_len + 1);
		memcpy(host, amqp_server, host_len);
		host[host_len] = 0;

		port = strtol(colon+1, &port_end, 10);
		if (port < 0 
		    || port > 65535
		    || port_end == colon+1
		    || *port_end != 0)
			die("bad server port number in %s", amqp_server);
	}

	s = amqp_open_socket(host, port ? port : 5672);
	die_amqp_error(s, "opening socket to %s", amqp_server);
	
	conn = amqp_new_connection();
	amqp_set_sockfd(conn, s);
	
	die_rpc(amqp_login(conn, amqp_vhost, 0, 131072, 0,
			   AMQP_SASL_METHOD_PLAIN,
			   amqp_username, amqp_password),
		"logging in to AMQP server");

	if (!amqp_channel_open(conn, 1))
		die_rpc(amqp_get_rpc_reply(conn), "opening channel");
		
	return conn;
}

void close_connection(amqp_connection_state_t conn)
{
	int res;
	die_rpc(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
		"closing channel");
	die_rpc(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
		"closing connection");
	
	res = amqp_destroy_connection(conn);
	die_amqp_error(res, "closing connection");
}

amqp_bytes_t read_all(int fd)
{
	size_t space = 4096;
	amqp_bytes_t bytes;

	bytes.bytes = malloc(space);
	bytes.len = 0;

	for (;;) {
		ssize_t res = read(fd, bytes.bytes+bytes.len, space-bytes.len);
		if (res == 0)
			break;

		if (res < 0) {
			if (errno == EINTR)
				continue;

			die_errno(errno, "reading");
		}

		bytes.len += res;
		if (bytes.len == space) {
			space *= 2;
			bytes.bytes = realloc(bytes.bytes, space);
		}
	}

	return bytes;
}

void write_all(int fd, amqp_bytes_t data)
{
	while (data.len > 0) {
		ssize_t res = write(fd, data.bytes, data.len);
		if (res < 0)
			die_errno(errno, "write");
		
		data.len -= res;
		data.bytes += res;
	}
}

void copy_body(amqp_connection_state_t conn, int fd)
{
	size_t body_remaining;
	amqp_frame_t frame;
		
	int res = amqp_simple_wait_frame(conn, &frame);
	die_amqp_error(res, "waiting for header frame");
	if (frame.frame_type != AMQP_FRAME_HEADER)
		die("expected header, got frame type 0x%X",
		    frame.frame_type);
	
	body_remaining = frame.payload.properties.body_size;
	while (body_remaining) {
		res = amqp_simple_wait_frame(conn, &frame);
		die_amqp_error(res, "waiting for body frame");
		if (frame.frame_type != AMQP_FRAME_BODY)
			die("expected body, got frame type 0x%X",
			    frame.frame_type);
		
		write_all(fd, frame.payload.body_fragment);
		body_remaining -= frame.payload.body_fragment.len;
	}
}

poptContext process_options(int argc, const char **argv,
			    struct poptOption *options,
			    const char *help)
{
	int c;
	poptContext opts = poptGetContext(NULL, argc, argv, options, 0);
	poptSetOtherOptionHelp(opts, help);

	while ((c = poptGetNextOpt(opts)) >= 0) {
		/* no options require explicit handling */
	}

	if (c < -1) {
		fprintf(stderr, "%s: %s\n",
			poptBadOption(opts, POPT_BADOPTION_NOALIAS),
			poptStrerror(c));
		poptPrintUsage(opts, stderr, 0);
		exit(1);
	}

	return opts;
}

void process_all_options(int argc, const char **argv,
			 struct poptOption *options)
{
	poptContext opts = process_options(argc, argv, options,
					   "[OPTIONS]...");
        const char *opt = poptPeekArg(opts);
	
	if (opt) {
		fprintf(stderr, "unexpected operand: %s\n", opt);
		poptPrintUsage(opts, stderr, 0);
		exit(1);
	}
	
	poptFreeContext(opts);
}

amqp_bytes_t cstring_bytes(const char *str)
{
	return str ? amqp_cstring_bytes(str) : AMQP_EMPTY_BYTES;
}
