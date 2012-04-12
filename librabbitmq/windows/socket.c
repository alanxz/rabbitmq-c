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
 * Portions created by VMware are Copyright (c) 2007-2012 VMware, Inc.
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
