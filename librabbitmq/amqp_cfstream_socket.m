/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
/*
 * Copyright 2015 Andrew Mackenzie-Ross
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef AMQP_CFSTREAM_SOCKET

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#import "amqp_cfstream_socket_objc.h"

#import <SystemConfiguration/SystemConfiguration.h>

#include "amqp_cfstream_socket.h"
#include "amqp_private.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

struct amqp_cfstream_socket_t {
    const struct amqp_socket_class_t *klass;
    CFWriteStreamRef write_stream;
    CFReadStreamRef read_stream;
    CFErrorRef write_error;
    CFErrorRef read_error;
    bool read_open;
    bool write_open;
    CFStringRef host;
    CFRunLoopRef run_loop;
    int port;
    void *buffer;
    size_t buffer_length;
    __unsafe_unretained void (^before_open_hook)(CFWriteStreamRef r, CFReadStreamRef w);
};

static const CFTimeInterval stream_activity_timeout = 30.0;

bool amqp_cfstream_socket_streams_are_opening_or_open(struct amqp_cfstream_socket_t *socket);
bool amqp_cfstream_socket_streams_are_open(struct amqp_cfstream_socket_t *socket);

bool amqp_cfstream_socket_stream_is_open(CFStreamStatus status);
bool amqp_cfstream_socket_stream_is_opening_or_open(CFStreamStatus status);

void amqp_cfstream_socket_write_stream_event_handler(CFWriteStreamRef stream, CFStreamEventType type, void *clientCallBackInfo);
void amqp_cfstream_socket_read_stream_event_handler(CFReadStreamRef stream, CFStreamEventType type, void *clientCallBackInfo);

bool amqp_cfstream_socket_streams_have_errors(struct amqp_cfstream_socket_t * socket);
void amqp_cfstream_socket_log_stream_errors(struct amqp_cfstream_socket_t * socket);
void amqp_cfstream_socket_close_streams(struct amqp_cfstream_socket_t *self);

static ssize_t
amqp_cfstream_socket_send_inner(void *base, const void *buf, size_t len)

{
    struct amqp_cfstream_socket_t *self = (struct amqp_cfstream_socket_t *)base;
    ssize_t res;
    const void *buf_left = buf;
    ssize_t len_left = len;
    
    if (!amqp_cfstream_socket_streams_are_opening_or_open(self)) {
        return AMQP_STATUS_SOCKET_CLOSED;
    }
    
start:
    while (true) {
        if (CFWriteStreamCanAcceptBytes(self->write_stream)) {
            break;
        }
        CFTimeInterval run_loop_time = (stream_activity_timeout > 0 ? stream_activity_timeout : [[NSDate distantFuture] timeIntervalSinceNow]);
        SInt32 run_loop_exit_reason = CFRunLoopRunInMode(kCFRunLoopDefaultMode, run_loop_time, true);
        if (run_loop_exit_reason != kCFRunLoopRunHandledSource) {
            return AMQP_STATUS_TIMEOUT;
        } else {
            if (amqp_cfstream_socket_streams_have_errors(self)) {
                 amqp_cfstream_socket_log_stream_errors(self);
                return AMQP_STATUS_SOCKET_ERROR;
            } else if (!amqp_cfstream_socket_streams_are_opening_or_open(self)) {
                return AMQP_STATUS_SOCKET_ERROR;
            }
        }
    }
    
    res = CFWriteStreamWrite(self->write_stream, buf_left, len_left);
    
    if (res < 0) {
        res = AMQP_STATUS_SOCKET_ERROR;
        amqp_cfstream_socket_log_stream_errors(self);
    } else {
        if (res == len_left) {
            res = AMQP_STATUS_OK;
        } else {
            buf_left += res;
            len_left -= res;
            goto start;
        }
    }
    
    return res;
}

static ssize_t
amqp_cfstream_socket_send(void *base, const void *buf, size_t len)
{
    return amqp_cfstream_socket_send_inner(base, buf, len);
}

static ssize_t
amqp_cfstream_socket_writev(void *base, struct iovec *iov, int iovcnt)
{
    struct amqp_cfstream_socket_t *self = (struct amqp_cfstream_socket_t *)base;
    ssize_t ret;
    if (!amqp_cfstream_socket_streams_are_opening_or_open(self)) {
        return AMQP_STATUS_SOCKET_CLOSED;
    }
    
    
    int i;
    size_t bytes = 0;
    void *bufferp;
    
    for (i = 0; i < iovcnt; ++i) {
        bytes += iov[i].iov_len;
    }
    
    if (self->buffer_length < bytes) {
        self->buffer = realloc(self->buffer, bytes);
        if (NULL == self->buffer) {
            self->buffer_length = 0;
            ret = AMQP_STATUS_NO_MEMORY;
            goto exit;
        }
        self->buffer_length = bytes;
    }
    
    bufferp = self->buffer;
    for (i = 0; i < iovcnt; ++i) {
        memcpy(bufferp, iov[i].iov_base, iov[i].iov_len);
        bufferp += iov[i].iov_len;
    }
    
    ret = amqp_cfstream_socket_send_inner(self, self->buffer, bytes);
    
exit:
    return ret;
    
}

static ssize_t
amqp_cfstream_socket_recv(void *base, void *buf, size_t len, int flags)
{
    struct amqp_cfstream_socket_t *self = (struct amqp_cfstream_socket_t *)base;
    ssize_t ret;
    if (!amqp_cfstream_socket_streams_are_opening_or_open(self)) {
        return AMQP_STATUS_SOCKET_CLOSED;
    }
    
start:
    ret = CFReadStreamRead(self->read_stream, buf, len);
    
    if (0 > ret) {
        ret = AMQP_STATUS_SOCKET_ERROR;
        amqp_cfstream_socket_log_stream_errors(self);
    } else if (0 == ret) {
        ret = AMQP_STATUS_CONNECTION_CLOSED;
    }
    return ret;
}

int
amqp_cfstream_socket_wait_timeout(amqp_connection_state_t state, uint64_t start, struct timeval *timeout) {
    struct amqp_cfstream_socket_t *self = (struct amqp_cfstream_socket_t *)state->socket;
    
    uint64_t end_timestamp = 0;
    if (timeout) {
        end_timestamp = start +
        (uint64_t)timeout->tv_sec * AMQP_NS_PER_S +
        (uint64_t)timeout->tv_usec * AMQP_NS_PER_US;
    }
    while(amqp_cfstream_socket_streams_are_open(self) && ! amqp_cfstream_socket_streams_have_errors(self)) {
        if (CFReadStreamHasBytesAvailable(self->read_stream)) {
            return AMQP_STATUS_OK;
        }
        CFTimeInterval run_loop_time;
        if (timeout) {
            uint64_t current_timestamp = amqp_get_monotonic_timestamp();
            if (0 == current_timestamp) {
                return AMQP_STATUS_TIMER_FAILURE;
            }
            if (current_timestamp > end_timestamp) {
                return AMQP_STATUS_TIMEOUT;
            }
            uint64_t time_left = end_timestamp - current_timestamp;
            run_loop_time = ((CFTimeInterval)time_left / (CFTimeInterval)AMQP_NS_PER_S);
        } else {
         run_loop_time = (stream_activity_timeout > 0 ? stream_activity_timeout : [[NSDate distantFuture] timeIntervalSinceNow]);
        }
        SInt32 run_loop_exit_reason = CFRunLoopRunInMode(kCFRunLoopDefaultMode, run_loop_time, true);
        if (run_loop_exit_reason != kCFRunLoopRunHandledSource) {
            return AMQP_STATUS_TIMEOUT;
        }
    }
    return AMQP_STATUS_SOCKET_ERROR;
}


static int
amqp_cfstream_socket_open(void *base, const char *host_c_str, int port, struct timeval *timeout_c)
{
    struct amqp_cfstream_socket_t *self = (struct amqp_cfstream_socket_t *)base;
    if (self->read_stream != nil) {
        return AMQP_STATUS_SOCKET_INUSE;
    }
    
    CFStringRef host = CFStringCreateWithCString(kCFAllocatorDefault, host_c_str, kCFStringEncodingUTF8);
    self->host = host;
    self->port = port;
    CFStreamCreatePairWithSocketToHost(kCFAllocatorDefault, host, port, &self->read_stream, &self->write_stream);
    
    if (self->read_stream == NULL || self->write_stream == NULL) {
        return AMQP_STATUS_TCP_SOCKETLIB_INIT_ERROR;
    }
    
    CFWriteStreamSetProperty(self->write_stream, kCFStreamPropertyShouldCloseNativeSocket, kCFBooleanTrue);
    CFReadStreamSetProperty(self->read_stream, kCFStreamPropertyShouldCloseNativeSocket, kCFBooleanTrue);
    
    if (self->before_open_hook) {
        self->before_open_hook(self->write_stream, self->read_stream);
    }
    
    CFStreamClientContext client_context = {0,self,NULL,NULL,NULL};
    CFWriteStreamSetClient(self->write_stream, kCFStreamEventOpenCompleted|kCFStreamEventErrorOccurred|kCFStreamEventEndEncountered|kCFStreamEventCanAcceptBytes,  amqp_cfstream_socket_write_stream_event_handler, &client_context);
    CFReadStreamSetClient(self->read_stream, kCFStreamEventOpenCompleted|kCFStreamEventErrorOccurred|kCFStreamEventEndEncountered|kCFStreamEventHasBytesAvailable,  amqp_cfstream_socket_read_stream_event_handler, &client_context);
    
    CFRunLoopRef rl = CFRunLoopGetCurrent();
    if (!rl) {
        [NSException raise:NSInternalInconsistencyException format:@"Cannot open cfstream for amqp_cfstream_socket on thread without runloop."];
    }
    CFRetain(rl);
    self->run_loop = rl;
    CFWriteStreamScheduleWithRunLoop(self->write_stream, rl, kCFRunLoopDefaultMode);
    CFReadStreamScheduleWithRunLoop(self->read_stream, rl, kCFRunLoopDefaultMode);
    
    
    if (!CFWriteStreamOpen(self->write_stream)) {
        return AMQP_STATUS_TCP_SOCKETLIB_INIT_ERROR;
    }
    
    if (!CFReadStreamOpen(self->read_stream)) {
        return AMQP_STATUS_TCP_SOCKETLIB_INIT_ERROR;
    }
    
    if (!timeout_c) {
        timeout_c = &(struct timeval){ .tv_sec = 30, .tv_usec = 0 };
    }
    
    uint64_t start = amqp_get_monotonic_timestamp();
    if (0 == start) {
        return AMQP_STATUS_TIMER_FAILURE;
    }
    uint64_t end_timestamp = start + (uint64_t)timeout_c->tv_sec * AMQP_NS_PER_S +
    (uint64_t)timeout_c->tv_usec * AMQP_NS_PER_US;
    while (amqp_cfstream_socket_streams_are_opening_or_open(self) && !amqp_cfstream_socket_streams_are_open(self)) {
        uint64_t current_timestamp = amqp_get_monotonic_timestamp();
        if (0 == current_timestamp) {
            return AMQP_STATUS_TIMER_FAILURE;
        }
        if (current_timestamp > end_timestamp) {
            return AMQP_STATUS_TIMEOUT;
        }
        uint64_t time_left = end_timestamp - current_timestamp;
        CFTimeInterval run_loop_time = ((CFTimeInterval)time_left / (CFTimeInterval)AMQP_NS_PER_S);
        SInt32 run_loop_exit_reason = CFRunLoopRunInMode(kCFRunLoopDefaultMode, run_loop_time, true);
        if (run_loop_exit_reason != kCFRunLoopRunHandledSource) {
            return AMQP_STATUS_TIMEOUT;
        } else {
            if ( amqp_cfstream_socket_streams_have_errors(self)) {
                 amqp_cfstream_socket_log_stream_errors(self);
                return AMQP_STATUS_SOCKET_ERROR;
            }
            if (amqp_cfstream_socket_streams_are_open(self)) {
                break;
            }
        }
    }
    
    return AMQP_STATUS_OK;
}

void
amqp_cfstream_socket_close_streams(struct amqp_cfstream_socket_t *self) {
    if (self->read_stream) {
        self->read_open = false;
        CFReadStreamUnscheduleFromRunLoop(self->read_stream, self->run_loop, kCFRunLoopDefaultMode);
        CFReadStreamClose(self->read_stream);
        self->read_stream = NULL;
    }
    if (self->write_stream) {
        self->write_open = false;
        CFWriteStreamUnscheduleFromRunLoop(self->write_stream, self->run_loop, kCFRunLoopDefaultMode);
        CFWriteStreamClose(self->write_stream);
        self->write_stream = NULL;
    }
    if (self->run_loop) {
        CFRelease(self->run_loop);
        self->run_loop = NULL;
    }
}

bool
amqp_cfstream_socket_streams_have_errors(struct amqp_cfstream_socket_t * socket) {
    return (socket->read_error || socket->write_error);
}


bool
amqp_cfstream_socket_streams_are_open(struct amqp_cfstream_socket_t *self) {
    if (!self->read_stream || !self->write_stream) {
        return false;
    }
    CFStreamStatus read_status = CFReadStreamGetStatus(self->read_stream);
    CFStreamStatus write_status = CFWriteStreamGetStatus(self->write_stream);
    
    return amqp_cfstream_socket_stream_is_open(read_status) && amqp_cfstream_socket_stream_is_open(write_status);
}

bool
amqp_cfstream_socket_streams_are_opening_or_open(struct amqp_cfstream_socket_t *self) {
    if (!self->read_stream || !self->write_stream) {
        return false;
    }
    CFStreamStatus read_status = CFReadStreamGetStatus(self->read_stream);
    CFStreamStatus write_status = CFWriteStreamGetStatus(self->write_stream);
    return  amqp_cfstream_socket_stream_is_opening_or_open(read_status) &&  amqp_cfstream_socket_stream_is_opening_or_open(write_status);
}

bool
amqp_cfstream_socket_stream_is_open(CFStreamStatus status) {
    return
    status == kCFStreamStatusOpen ||
    status == kCFStreamStatusReading ||
    status == kCFStreamStatusWriting;
}

bool
amqp_cfstream_socket_stream_is_opening_or_open(CFStreamStatus status) {
    return
    status == kCFStreamStatusOpen ||
    status == kCFStreamStatusOpening ||
    status == kCFStreamStatusReading ||
    status == kCFStreamStatusWriting;
}

void
amqp_cfstream_socket_log_stream_errors(struct amqp_cfstream_socket_t * socket) {
    if (socket->read_error) {
        NSLog(@"stream read error: %@", socket->read_error);
    }
    if (socket->write_error) {
        NSLog(@"stream write error: %@", socket->write_error);
    }
}

void
amqp_cfstream_socket_write_stream_event_handler(CFWriteStreamRef stream, CFStreamEventType type, void *clientCallBackInfo) {
    struct amqp_cfstream_socket_t * socketInfo = (struct amqp_cfstream_socket_t *)clientCallBackInfo;
    switch (type) {
        case kCFStreamEventErrorOccurred:
            socketInfo->write_error = CFWriteStreamCopyError(stream);
            amqp_cfstream_socket_close_streams(socketInfo);
            break;
        case kCFStreamEventOpenCompleted:
            socketInfo->write_open = true;
            break;
        case kCFStreamEventEndEncountered:
            socketInfo->write_open = false;
            break;
        default:
            break;
    }
}

void
amqp_cfstream_socket_read_stream_event_handler(CFReadStreamRef stream, CFStreamEventType type, void *clientCallBackInfo) {
    struct amqp_cfstream_socket_t * socketInfo = (struct amqp_cfstream_socket_t *)clientCallBackInfo;
    switch (type) {
        case kCFStreamEventErrorOccurred:
            socketInfo->read_error = CFReadStreamCopyError(stream);
            amqp_cfstream_socket_close_streams(socketInfo);
        break;
            break;
        case kCFStreamEventOpenCompleted:
            socketInfo->read_open = true;
            break;
        case kCFStreamEventEndEncountered:
            socketInfo->read_open = false;
            break;
        default:
            break;
    }
}

static int
amqp_cfstream_socket_close(void *base)
{
    struct amqp_cfstream_socket_t *self = (struct amqp_cfstream_socket_t *)base;
    if (!self->read_stream || !self->write_stream) {
        return AMQP_STATUS_SOCKET_CLOSED;
    }
    
    amqp_cfstream_socket_close_streams(self);
    
    return AMQP_STATUS_OK;
}

static int
amqp_cfstream_socket_get_sockfd(void *base)
{
    [NSException raise:NSInternalInconsistencyException format:@"Cannot call get_sockfd on cfstream backed socket."];
    return -1;
}

static void
amqp_cfstream_socket_delete(void *base)
{
    struct amqp_cfstream_socket_t *self = (struct amqp_cfstream_socket_t *)base;
    
    if (self) {
        amqp_cfstream_socket_close(self);
        
        if (self->read_error) {
            CFRelease(self->read_error);
            self->read_error = nil;
        }
        if (self->write_error) {
            CFRelease(self->write_error);
            self->write_error = nil;
        }
        
        [self->before_open_hook release];
        self->before_open_hook = nil;
        
        CFRelease(self->host);
        self->host = nil;
        
        free(self->buffer);
        free(self);
    }
}

static const struct amqp_socket_class_t amqp_cfstream_socket_class = {
    amqp_cfstream_socket_writev, /* writev */
    amqp_cfstream_socket_send, /* send */
    amqp_cfstream_socket_recv, /* recv */
    amqp_cfstream_socket_open, /* open */
    amqp_cfstream_socket_close, /* close */
    amqp_cfstream_socket_get_sockfd, /* get_sockfd */
    amqp_cfstream_socket_delete /* delete */
};

amqp_socket_t *
amqp_cfstream_socket_new(amqp_connection_state_t state, void(^before_open_hook)(CFWriteStreamRef write_stream, CFReadStreamRef read_stream))
{
    struct amqp_cfstream_socket_t *self = calloc(1, sizeof(*self));
    if (!self) {
        return NULL;
    }
    self->klass = &amqp_cfstream_socket_class;
    
    // copy block to heap and increment retain count
    self->before_open_hook = [before_open_hook copy];
    
    
    amqp_set_socket(state, (amqp_socket_t *)self);
    
    return (amqp_socket_t *)self;
}

int amqp_using_cfstream_socket(amqp_socket_t *self) {
    return (self->klass == &amqp_cfstream_socket_class);
}

CFReadStreamRef
amqp_cfstream_socket_get_read_stream(amqp_socket_t *base)
{
    if (!amqp_using_cfstream_socket(base)) {
        [NSException raise:NSInternalInconsistencyException format:@"Socket type must be cfstream."];
    }
    struct amqp_cfstream_socket_t *self = (struct amqp_cfstream_socket_t *)base;
    return self->read_stream;
}

CFWriteStreamRef
amqp_cfstream_socket_get_write_stream(amqp_socket_t *base)
{
    if (!amqp_using_cfstream_socket(base)) {
        [NSException raise:NSInternalInconsistencyException format:@"Socket type must be cfstream."];
    }
    struct amqp_cfstream_socket_t *self = (struct amqp_cfstream_socket_t *)base;
    return self->write_stream;
}

#endif /* AMQP_CFSTREAM_SOCKET */