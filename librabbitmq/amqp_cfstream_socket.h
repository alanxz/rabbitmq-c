/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
/** \file */
/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
 *
 * Portions created by Andrew Mackenzie-Ross are Copyright (c) 2015 Andrew Mackenzie-Ross.
 * All Rights Reserved.
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

#ifdef AMQP_CFSTREAM_SOCKET

#ifndef AMQP_CFSTREAM_SOCKET_H
#define AMQP_CFSTREAM_SOCKET_H

#include "amqp.h"

AMQP_BEGIN_DECLS

int
amqp_using_cfstream_socket(amqp_socket_t *self);

int
amqp_cfstream_socket_wait_timeout(amqp_connection_state_t state, uint64_t start, struct timeval *timeout);

AMQP_END_DECLS

#endif /* AMQP_CFSTREAM_SOCKET_H */

#endif /* AMQP_CFSTREAM_SOCKET */