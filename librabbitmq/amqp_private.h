#ifndef librabbitmq_amqp_private_h
#define librabbitmq_amqp_private_h

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "amqp.h"
#include "amqp_framing.h"
#include <string.h>

/* Error numbering: Because of differences in error numbering on
 * different platforms, we want to keep error numbers opaque for
 * client code.  Internally, we encode the category of an error
 * (i.e. where its number comes from) in the top bits of the number
 * (assuming that an int has at least 32 bits).
 */
#define ERROR_CATEGORY_MASK (1 << 29)

#define ERROR_CATEGORY_CLIENT (0 << 29) /* librabbitmq error codes */
#define ERROR_CATEGORY_OS (1 << 29) /* OS-specific error codes */

/* librabbitmq error codes */
#define ERROR_NO_MEMORY 1
#define ERROR_BAD_AMQP_DATA 2
#define ERROR_UNKNOWN_CLASS 3
#define ERROR_UNKNOWN_METHOD 4
#define ERROR_GETHOSTBYNAME_FAILED 5
#define ERROR_INCOMPATIBLE_AMQP_VERSION 6
#define ERROR_CONNECTION_CLOSED 7
#define ERROR_BAD_AMQP_URL 8
#define ERROR_MAX 8

/* GCC attributes */
#if __GNUC__ > 2 | (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define AMQP_NORETURN \
  __attribute__ ((__noreturn__))
#else
#define AMQP_NORETURN
#endif

#if __GNUC__ >= 4
#define AMQP_PRIVATE \
  __attribute__ ((visibility ("hidden")))
#else
#define AMQP_PRIVATE
#endif

char *
amqp_os_error_string(int err);

#include "socket.h"

/*
 * Connection states: XXX FIX THIS
 *
 * - CONNECTION_STATE_INITIAL: The initial state, when we cannot be
 *   sure if the next thing we will get is the first AMQP frame, or a
 *   protocol header from the server.
 *
 * - CONNECTION_STATE_IDLE: The normal state between
 *   frames. Connections may only be reconfigured, and the
 *   connection's pools recycled, when in this state. Whenever we're
 *   in this state, the inbound_buffer's bytes pointer must be NULL;
 *   any other state, and it must point to a block of memory allocated
 *   from the frame_pool.
 *
 * - CONNECTION_STATE_HEADER: Some bytes of an incoming frame have
 *   been seen, but not a complete frame header's worth.
 *
 * - CONNECTION_STATE_BODY: A complete frame header has been seen, but
 *   the frame is not yet complete. When it is completed, it will be
 *   returned, and the connection will return to IDLE state.
 *
 */
typedef enum amqp_connection_state_enum_ {
  CONNECTION_STATE_IDLE = 0,
  CONNECTION_STATE_INITIAL,
  CONNECTION_STATE_HEADER,
  CONNECTION_STATE_BODY
} amqp_connection_state_enum;

/* 7 bytes up front, then payload, then 1 byte footer */
#define HEADER_SIZE 7
#define FOOTER_SIZE 1

#define AMQP_PSEUDOFRAME_PROTOCOL_HEADER 'A'

typedef struct amqp_link_t_ {
  struct amqp_link_t_ *next;
  void *data;
} amqp_link_t;

struct amqp_connection_state_t_ {
  amqp_pool_t frame_pool;
  amqp_pool_t decoding_pool;

  amqp_connection_state_enum state;

  int channel_max;
  int frame_max;
  int heartbeat;
  amqp_bytes_t inbound_buffer;

  size_t inbound_offset;
  size_t target_size;

  amqp_bytes_t outbound_buffer;

  int sockfd;
  amqp_bytes_t sock_inbound_buffer;
  size_t sock_inbound_offset;
  size_t sock_inbound_limit;

  amqp_link_t *first_queued_frame;
  amqp_link_t *last_queued_frame;

  amqp_rpc_reply_t most_recent_api_result;
};

static inline void *amqp_offset(void *data, size_t offset)
{
  return (char *)data + offset;
}

/* This macro defines the encoding and decoding functions associated with a
   simple type. */

#define DECLARE_CODEC_BASE_TYPE(bits, htonx, ntohx)                         \
                                                                            \
static inline void amqp_e##bits(void *data, size_t offset,                  \
                                uint##bits##_t val)                         \
{									    \
  /* The AMQP data might be unaligned. So we encode and then copy the       \
     result into place. */		   				    \
  uint##bits##_t res = htonx(val);	   				    \
  memcpy(amqp_offset(data, offset), &res, bits/8);                          \
}                                                                           \
                                                                            \
static inline uint##bits##_t amqp_d##bits(void *data, size_t offset)        \
{			      		   				    \
  /* The AMQP data might be unaligned.  So we copy the source value	    \
     into a variable and then decode it. */				    \
  uint##bits##_t val;	      						    \
  memcpy(&val, amqp_offset(data, offset), bits/8);                          \
  return ntohx(val);							    \
}                                                                           \
                                                                            \
static inline int amqp_encode_##bits(amqp_bytes_t encoded, size_t *offset,  \
                                     uint##bits##_t input)                  \
                                                                            \
{                                                                           \
  size_t o = *offset;                                                       \
  if ((*offset = o + bits / 8) <= encoded.len) {                            \
    amqp_e##bits(encoded.bytes, o, input);                                  \
    return 1;                                                               \
  }                                                                         \
  else {                                                                    \
    return 0;                                                               \
  }                                                                         \
}                                                                           \
                                                                            \
static inline int amqp_decode_##bits(amqp_bytes_t encoded, size_t *offset,  \
                                     uint##bits##_t *output)                \
                                                                            \
{                                                                           \
  size_t o = *offset;                                                       \
  if ((*offset = o + bits / 8) <= encoded.len) {                            \
    *output = amqp_d##bits(encoded.bytes, o);                               \
    return 1;                                                               \
  }                                                                         \
  else {                                                                    \
    return 0;                                                               \
  }                                                                         \
}

/* Determine byte order */
#if defined(__GLIBC__)
# include <endian.h>
# if (__BYTE_ORDER == __LITTLE_ENDIAN)
#  define AMQP_LITTLE_ENDIAN
# elif (__BYTE_ORDER == __BIG_ENDIAN)
#  define AMQP_BIG_ENDIAN
# else
   /* Don't define anything */
# endif
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN) ||                   \
      defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
# define AMQP_BIG_ENDIAN
#elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN) ||                   \
      defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
# define AMQP_LITTLE_ENDIAN
#elif defined(__hppa__) || defined(__HPPA__) || defined(__hppa) ||          \
      defined(_POWER) || defined(__powerpc__) || defined(__ppc___) ||       \
      defined(_MIPSEB) || defined(__s390__) ||                              \
      defined(__sparc) || defined(__sparc__)
# define AMQP_BIG_ENDIAN
#elif defined(__alpha__) || defined(__alpha) || defined(_M_ALPHA) ||        \
      defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) ||       \
      defined(__ia64) || defined(__ia64__) || defined(_M_IA64) ||           \
      defined(__arm__) || defined(_M_ARM) ||                                \
      defined(__i386__) || defined(_M_IX86)
# define AMQP_LITTLE_ENDIAN
#else
  /* Don't define anything */
#endif

#if defined(AMQP_LITTLE_ENDIAN)

#define DECLARE_XTOXLL(func)                      \
static inline uint64_t func##ll(uint64_t val)     \
{                                                 \
  union {                                         \
    uint64_t whole;                               \
    uint32_t halves[2];                           \
  } u;                                            \
  uint32_t t;                                     \
  u.whole = val;                                  \
  t = u.halves[0];                                \
  u.halves[0] = func##l(u.halves[1]);             \
  u.halves[1] = func##l(t);                       \
  return u.whole;                                 \
}

#elif defined(AMQP_BIG_ENDIAN)

#define DECLARE_XTOXLL(func)                      \
static inline uint64_t func##ll(uint64_t val)     \
{                                                 \
  union {                                         \
    uint64_t whole;                               \
    uint32_t halves[2];                           \
  } u;                                            \
  u.whole = val;                                  \
  u.halves[0] = func##l(u.halves[0]);             \
  u.halves[1] = func##l(u.halves[1]);             \
  return u.whole;                                 \
}

#else
# error Endianness not known
#endif

#ifndef HAVE_HTONLL
DECLARE_XTOXLL(hton)
DECLARE_XTOXLL(ntoh)
#endif

DECLARE_CODEC_BASE_TYPE(8, (uint8_t), (uint8_t))
DECLARE_CODEC_BASE_TYPE(16, htons, ntohs)
DECLARE_CODEC_BASE_TYPE(32, htonl, ntohl)
DECLARE_CODEC_BASE_TYPE(64, htonll, ntohll)

static inline int amqp_encode_bytes(amqp_bytes_t encoded, size_t *offset,
				    amqp_bytes_t input)
{
  size_t o = *offset;
  if ((*offset = o + input.len) <= encoded.len) {
    memcpy(amqp_offset(encoded.bytes, o), input.bytes, input.len);
    return 1;
  }
  else {
    return 0;
  }
}

static inline int amqp_decode_bytes(amqp_bytes_t encoded, size_t *offset,
				    amqp_bytes_t *output, size_t len)
{
  size_t o = *offset;
  if ((*offset = o + len) <= encoded.len) {
    output->bytes = amqp_offset(encoded.bytes, o);
    output->len = len;
    return 1;
  }
  else {
    return 0;
  }
}

AMQP_NORETURN
void
amqp_abort(const char *fmt, ...);

#endif
