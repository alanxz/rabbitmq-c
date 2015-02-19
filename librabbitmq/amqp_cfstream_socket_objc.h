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

#import <Foundation/Foundation.h>
#import "amqp.h"

AMQP_BEGIN_DECLS

/**
 * Create a new TCP socket.
 *
 * Call amqp_connection_close() to release socket resources.
 *
 * \return A new socket object or NULL if an error occurred.
 *
 * \since v0.7.0
 */
AMQP_PUBLIC_FUNCTION
amqp_socket_t *
AMQP_CALL
amqp_cfstream_socket_new(amqp_connection_state_t state, void(^before_open_hook)(CFWriteStreamRef write, CFReadStreamRef read_stream));


AMQP_END_DECLS

#endif /* AMQP_CFSTREAM_SOCKET */
