/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
 *
 * Portions created by Alan Antonuk are Copyright (c) 2012-2013
 * Alan Antonuk. All Rights Reserved.
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

#include "amqp_private.h"
#include "socket.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
amqp_socket_init(void)
{
  return AMQP_STATUS_OK;
}

int
amqp_socket_socket(int domain, int type, int proto)
{
  int flags;

  int s = socket(domain, type, proto);
  if (s < 0) {
    return s;
  }

  /* Always enable CLOEXEC on the socket */
  flags = fcntl(s, F_GETFD);
  if (flags == -1
      || fcntl(s, F_SETFD, (long)(flags | FD_CLOEXEC)) == -1) {
    int e = errno;
    close(s);
    errno = e;
    return -1;
  }

  return s;
}

char *
amqp_os_error_string(int err)
{
  return strdup(strerror(err));
}

int
amqp_os_socket_close(int sockfd)
{
  return close(sockfd);
}

ssize_t
amqp_os_socket_writev(int sockfd, const struct iovec *iov,
                      int iovcnt)
{
  return writev(sockfd, iov, iovcnt);
}

int
amqp_os_socket_error(void)
{
  return errno;
}
