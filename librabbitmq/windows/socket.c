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

/* See http://msdn.microsoft.com/en-us/library/ms737629%28VS.85%29.aspx */
#define WIN32_LEAN_AND_MEAN

#include "config.h"

#include <windows.h>
#include <stdint.h>
#include <stdlib.h>

#include "amqp.h"
#include "amqp_private.h"
#include "socket.h"

static int called_wsastartup;

int amqp_socket_init(void)
{
	if (!called_wsastartup) {
		WSADATA data;
		int res = WSAStartup(0x0202, &data);
		if (res)
			return -res;

		called_wsastartup = 1;
	}

	return 0;
}

char *amqp_os_error_string(int err)
{
	char *msg, *copy;

	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
			       | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			   NULL, err,
			   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			   (LPSTR)&msg, 0, NULL))
		return strdup("(error retrieving Windows error message)");

	copy = strdup(msg);
	LocalFree(msg);
	return copy;
}
