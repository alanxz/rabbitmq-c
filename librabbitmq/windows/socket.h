#ifndef librabbitmq_windows_socket_h
#define librabbitmq_windows_socket_h

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

#include <winsock2.h>

extern int amqp_socket_init(void);

#define amqp_socket_socket socket
#define amqp_socket_close closesocket

static inline int amqp_socket_setsockopt(int sock, int level, int optname,
                                    const void *optval, size_t optlen)
{
        /* the winsock setsockopt function has its 4th argument as a
           const char * */
        return setsockopt(sock, level, optname, (const char *)optval, optlen);
}

/* same as WSABUF */
struct iovec {
	u_long iov_len;
	void *iov_base;
};

static inline int amqp_socket_writev(int sock, struct iovec *iov, int nvecs)
{
	DWORD ret;
	if (WSASend(sock, (LPWSABUF)iov, nvecs, &ret, 0, NULL, NULL) == 0)
		return ret;
	else
		return -1;
}

static inline int amqp_socket_error()
{
	return WSAGetLastError() | ERROR_CATEGORY_OS;
}

#endif
