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
#include <string.h>
#include <stdlib.h>

#include <inttypes.h>

#include <amqp.h>

static void match_string(const char *what, const char *expect, const char *got)
{
	if (strcmp(got, expect)) {
		fprintf(stderr, "Expected %s '%s', got '%s'\n",
			what, expect, got);
		abort();
	}
}

static void match_int(const char *what, int expect, int got)
{
	if (got != expect) {
		fprintf(stderr, "Expected %s '%d', got '%d'\n",
			what, expect, got);
		abort();
	}
}

static void parse_success(const char *url,
			  const char *user,
			  const char *password,
			  const char *host,
			  int port,
			  const char *vhost)
{
	char *s = strdup(url);
	struct amqp_connection_info ci;
	int res;

	amqp_default_connection_info(&ci);
	res = amqp_parse_url(s, &ci);
	if (res) {
		fprintf(stderr,
		   "Expected to successfully parse URL, but didn't: %s (%s)\n",
		   url, amqp_error_string(-res));
		abort();
	}

	match_string("user", user, ci.user);
	match_string("password", password, ci.password);
	match_string("host", host, ci.host);
	match_int("port", port, ci.port);
	match_string("vhost", vhost, ci.vhost);

	free(s);
}

static void parse_fail(const char *url)
{
	char *s = strdup(url);
	struct amqp_connection_info ci;

	amqp_default_connection_info(&ci);
	if (amqp_parse_url(s, &ci) >= 0) {
		fprintf(stderr,
			"Expected to fail parsing URL, but didn't: %s\n",
			url);
		abort();
	}

	free(s);
}

int main(int argc, char **argv)
{
	/* From the spec */
	parse_success("amqp://user:pass@host:10000/vhost", "user", "pass",
		      "host", 10000, "vhost");
	parse_success("amqp://user%61:%61pass@ho%61st:10000/v%2fhost",
		      "usera", "apass", "hoast", 10000, "v/host");
	parse_success("amqp://", "guest", "guest", "localhost", 5672, "/");
	parse_success("amqp://:@/", "", "", "localhost", 5672, "");
	parse_success("amqp://user@", "user", "guest", "localhost", 5672, "/");
	parse_success("amqp://user:pass@", "user", "pass",
		      "localhost", 5672, "/");
	parse_success("amqp://host", "guest", "guest", "host", 5672, "/");
	parse_success("amqp://:10000", "guest", "guest", "localhost", 10000,
		      "/");
	parse_success("amqp:///vhost", "guest", "guest", "localhost", 5672,
		      "vhost");
	parse_success("amqp://host/", "guest", "guest", "host", 5672, "");
	parse_success("amqp://host/%2f", "guest", "guest", "host", 5672, "/");
	parse_success("amqp://[::1]", "guest", "guest", "::1", 5672, "/");

	/* Various other success cases */
	parse_success("amqp://host:100", "guest", "guest", "host", 100, "/");
	parse_success("amqp://[::1]:100", "guest", "guest", "::1", 100, "/");

	parse_success("amqp://host/blah", "guest", "guest",
		      "host", 5672, "blah");
	parse_success("amqp://host:100/blah", "guest", "guest",
		      "host", 100, "blah");
	parse_success("amqp://:100/blah", "guest", "guest",
		      "localhost", 100, "blah");
	parse_success("amqp://[::1]/blah", "guest", "guest",
		      "::1", 5672, "blah");
	parse_success("amqp://[::1]:100/blah", "guest", "guest",
		      "::1", 100, "blah");

	parse_success("amqp://user:pass@host", "user", "pass",
		      "host", 5672, "/");
	parse_success("amqp://user:pass@host:100", "user", "pass",
		      "host", 100, "/");
	parse_success("amqp://user:pass@:100", "user", "pass",
		      "localhost", 100, "/");
	parse_success("amqp://user:pass@[::1]", "user", "pass",
		      "::1", 5672, "/");
	parse_success("amqp://user:pass@[::1]:100", "user", "pass",
		      "::1", 100, "/");

	/* Various failure cases */
	parse_fail("http://www.rabbitmq.com");
	parse_fail("amqp://foo:bar:baz");
	parse_fail("amqp://foo[::1]");
	parse_fail("amqp://foo:[::1]");
	parse_fail("amqp://[::1]foo");
	parse_fail("amqp://foo:1000xyz");
	parse_fail("amqp://foo:1000000");
	parse_fail("amqp://foo/bar/baz");

	parse_fail("amqp://foo%1");
	parse_fail("amqp://foo%1x");
	parse_fail("amqp://foo%xy");

	return 0;
}
