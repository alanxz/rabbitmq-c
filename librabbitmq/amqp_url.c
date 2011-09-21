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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "amqp.h"
#include "amqp_framing.h"
#include "amqp_private.h"

void amqp_default_connection_info(struct amqp_connection_info *ci)
{
	/* Apply defaults */
	ci->user = "guest";
	ci->password = "guest";
	ci->host = "localhost";
	ci->port = 5672;
	ci->vhost = "/";
}

/* Scan for the next delimiter, handling percent-encodings on the way. */
static char find_delim(char **pp, int colon_and_at_sign_are_delims)
{
	char *from = *pp;
	char *to = from;

	for (;;) {
		char ch = *from++;

		switch (ch) {
		case ':':
		case '@':
			if (!colon_and_at_sign_are_delims) {
				*to++ = ch;
				break;
			}

			/* fall through */
		case 0:
		case '/':
		case '?':
		case '#':
		case '[':
		case ']':
			*to = 0;
			*pp = from;
			return ch;

		case '%': {
			unsigned int val;
			int chars;
			int res = sscanf(from, "%2x%n", &val, &chars);

			if (res == EOF || res < 1 || chars != 2)
				/* Return a surprising delimiter to
				   force an error. */
				return '%';

			*to++ = val;
			from += 2;
			break;
		}

		default:
			*to++ = ch;
			break;
		}
	}
}

/* Parse an AMQP URL into its component parts. */
int amqp_parse_url(char *url, struct amqp_connection_info *parsed)
{
	int res = -ERROR_BAD_AMQP_URL;
	char delim;
	char *start;
	char *host;
	char *port = NULL;

	/* check the prefix */
	if (strncmp(url, "amqp://", 7))
		goto out;

	host = start = url += 7;
	delim = find_delim(&url, 1);

	if (delim == ':') {
		/* The colon could be introducing the port or the
		   password part of the userinfo.  We don't know yet,
		   so stash the preceding component. */
		port = start = url;
		delim = find_delim(&url, 1);
	}

	if (delim == '@') {
		/* What might have been the host and port were in fact
		   the username and password */
		parsed->user = host;
		if (port)
			parsed->password = port;

		port = NULL;
		host = start = url;
		delim = find_delim(&url, 1);
	}

	if (delim == '[') {
		/* IPv6 address.  The bracket should be the first
		   character in the host. */
		if (host != start || *host != 0)
			goto out;

		start = url;
		delim = find_delim(&url, 0);

		if (delim != ']')
			goto out;

		parsed->host = start;
		start = url;
		delim = find_delim(&url, 1);

		/* Closing bracket should be the last character in the
		   host. */
		if (*start != 0)
			goto out;
	}
	else {
		/* If we haven't seen the host yet, this is it. */
		if (*host != 0)
			parsed->host = host;
	}

	if (delim == ':') {
		port = start = url;
		delim = find_delim(&url, 1);
	}

	if (port) {
		char *end;
		long portnum = strtol(port, &end, 10);

		if (port == end || *end != 0 || portnum < 0 || portnum > 65535)
			goto out;

		parsed->port = portnum;
	}

	if (delim == '/') {
		start = url;
		delim = find_delim(&url, 1);

		if (delim != 0)
			goto out;

		parsed->vhost = start;
		res = 0;
	}
	else if (delim == 0) {
		res = 0;
	}

	/* Any other delimiter is bad, and we will return
	   ERROR_BAD_AMQP_URL. */

 out:
	return res;
}
