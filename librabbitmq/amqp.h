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

#ifndef AMQP_H
#define AMQP_H

#ifdef __cplusplus
#define AMQP_BEGIN_DECLS extern "C" {
#define AMQP_END_DECLS }
#else
#define AMQP_BEGIN_DECLS
#define AMQP_END_DECLS
#endif

/** Important API Decorators
  * AMQP_PUBLIC_FUNCTION - Declares an exportable function
  * AMQP_PUBLIC_VARIABLE - Declares an exportable variable
  * AMQP_CALL - Declares the calling convention
  */

#if defined(_WIN32) && defined(_MSC_VER)
# if defined(AMQP_BUILD) && !defined(AMQP_STATIC)
#  define AMQP_PUBLIC_FUNCTION __declspec(dllexport)
#  define AMQP_PUBLIC_VARIABLE __declspec(dllexport) extern
# else
#  define AMQP_PUBLIC_FUNCTION
#  if !defined(AMQP_STATIC)
#   define AMQP_PUBLIC_VARIABLE __declspec(dllimport) extern
#  else
#   define AMQP_PUBLIC_VARIABLE extern
#  endif
# endif
# define AMQP_CALL __cdecl

#elif defined(_WIN32) && defined(__BORLANDC__)
# if defined(AMQP_BUILD) && !defined(AMQP_STATIC)
#  define AMQP_PUBLIC_FUNCTION __declspec(dllexport)
#  define AMQP_PUBLIC_VARIABLE __declspec(dllexport) extern
# else
#  define AMQP_PUBLIC_FUNCTION
#  if !defined(AMQP_STATIC)
#   define AMQP_PUBLIC_VARIABLE __declspec(dllimport) extern
#  else
#   define AMQP_PUBLIC_VARIABLE extern
#  endif
# endif
# define AMQP_CALL __cdecl

#elif defined(_WIN32) && defined(__MINGW32__)
# if defined(AMQP_BUILD) && !defined(AMQP_STATIC)
#  define AMQP_PUBLIC_FUNCTION __declspec(dllexport)
#  define AMQP_PUBLIC_VARIABLE __declspec(dllexport)
# else
#  define AMQP_PUBLIC_FUNCTION
#  if !defined(AMQP_STATIC)
#   define AMQP_PUBLIC_VARIABLE __declspec(dllimport) extern
#  else
#   define AMQP_PUBLIC_VARIABLE extern
#  endif
# endif
# define AMQP_CALL __cdecl

#elif defined(_WIN32) && defined(__CYGWIN__)
# if defined(AMQP_BUILD) && !defined(AMQP_STATIC)
#  define AMQP_PUBLIC_FUNCTION __declspec(dllexport)
#  define AMQP_PUBLIC_VARIABLE __declspec(dllexport)
# else
#  define AMQP_PUBLIC_FUNCTION
#  if !defined(AMQP_STATIC)
#   define AMQP_PUBLIC_VARIABLE __declspec(dllimport) extern
#  else
#   define AMQP_PUBLIC_VARIABLE extern
#  endif
# endif
# define AMQP_CALL __cdecl

#elif defined(__GNUC__) && __GNUC__ >= 4
# define AMQP_PUBLIC_FUNCTION \
  __attribute__ ((visibility ("default")))
# define AMQP_PUBLIC_VARIABLE \
  __attribute__ ((visibility ("default"))) extern
# define AMQP_CALL
#else
# define AMQP_PUBLIC_FUNCTION
# define AMQP_PUBLIC_VARIABLE extern
# define AMQP_CALL
#endif

#include <stddef.h>
#include <stdint.h>

AMQP_BEGIN_DECLS

typedef int amqp_boolean_t;
typedef uint32_t amqp_method_number_t;
typedef uint32_t amqp_flags_t;
typedef uint16_t amqp_channel_t;

typedef struct amqp_bytes_t_ {
  size_t len;
  void *bytes;
} amqp_bytes_t;

typedef struct amqp_decimal_t_ {
  uint8_t decimals;
  uint32_t value;
} amqp_decimal_t;

typedef struct amqp_table_t_ {
  int num_entries;
  struct amqp_table_entry_t_ *entries;
} amqp_table_t;

typedef struct amqp_array_t_ {
  int num_entries;
  struct amqp_field_value_t_ *entries;
} amqp_array_t;

/*
  0-9   0-9-1   Qpid/Rabbit  Type               Remarks
---------------------------------------------------------------------------
        t       t            Boolean
        b       b            Signed 8-bit
        B                    Unsigned 8-bit
        U       s            Signed 16-bit	(A1)
        u                    Unsigned 16-bit
  I     I       I	     Signed 32-bit
        i		     Unsigned 32-bit
        L       l	     Signed 64-bit	(B)
        l		     Unsigned 64-bit
        f       f	     32-bit float
        d       d	     64-bit float
  D     D       D	     Decimal
        s		     Short string	(A2)
  S     S       S	     Long string
        A		     Nested Array
  T     T       T	     Timestamp (u64)
  F     F       F	     Nested Table
  V     V       V	     Void
                x	     Byte array

Remarks:

 A1, A2: Notice how the types **CONFLICT** here. In Qpid and Rabbit,
         's' means a signed 16-bit integer; in 0-9-1, it means a
	 short string.

 B: Notice how the signednesses **CONFLICT** here. In Qpid and Rabbit,
    'l' means a signed 64-bit integer; in 0-9-1, it means an unsigned
    64-bit integer.

I'm going with the Qpid/Rabbit types, where there's a conflict, and
the 0-9-1 types otherwise. 0-8 is a subset of 0-9, which is a subset
of the other two, so this will work for both 0-8 and 0-9-1 branches of
the code.
*/

typedef struct amqp_field_value_t_ {
  uint8_t kind;
  union {
    amqp_boolean_t boolean;
    int8_t i8;
    uint8_t u8;
    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;
    float f32;
    double f64;
    amqp_decimal_t decimal;
    amqp_bytes_t bytes;
    amqp_table_t table;
    amqp_array_t array;
  } value;
} amqp_field_value_t;

typedef struct amqp_table_entry_t_ {
  amqp_bytes_t key;
  amqp_field_value_t value;
} amqp_table_entry_t;

typedef enum {
  AMQP_FIELD_KIND_BOOLEAN = 't',
  AMQP_FIELD_KIND_I8 = 'b',
  AMQP_FIELD_KIND_U8 = 'B',
  AMQP_FIELD_KIND_I16 = 's',
  AMQP_FIELD_KIND_U16 = 'u',
  AMQP_FIELD_KIND_I32 = 'I',
  AMQP_FIELD_KIND_U32 = 'i',
  AMQP_FIELD_KIND_I64 = 'l',
  AMQP_FIELD_KIND_U64 = 'L',
  AMQP_FIELD_KIND_F32 = 'f',
  AMQP_FIELD_KIND_F64 = 'd',
  AMQP_FIELD_KIND_DECIMAL = 'D',
  AMQP_FIELD_KIND_UTF8 = 'S',
  AMQP_FIELD_KIND_ARRAY = 'A',
  AMQP_FIELD_KIND_TIMESTAMP = 'T',
  AMQP_FIELD_KIND_TABLE = 'F',
  AMQP_FIELD_KIND_VOID = 'V',
  AMQP_FIELD_KIND_BYTES = 'x'
} amqp_field_value_kind_t;

typedef struct amqp_pool_blocklist_t_ {
  int num_blocks;
  void **blocklist;
} amqp_pool_blocklist_t;

typedef struct amqp_pool_t_ {
  size_t pagesize;

  amqp_pool_blocklist_t pages;
  amqp_pool_blocklist_t large_blocks;

  int next_page;
  char *alloc_block;
  size_t alloc_used;
} amqp_pool_t;

typedef struct amqp_method_t_ {
  amqp_method_number_t id;
  void *decoded;
} amqp_method_t;

typedef struct amqp_frame_t_ {
  uint8_t frame_type; /* 0 means no event */
  amqp_channel_t channel;
  union {
    amqp_method_t method;
    struct {
      uint16_t class_id;
      uint64_t body_size;
      void *decoded;
      amqp_bytes_t raw;
    } properties;
    amqp_bytes_t body_fragment;
    struct {
      uint8_t transport_high;
      uint8_t transport_low;
      uint8_t protocol_version_major;
      uint8_t protocol_version_minor;
    } protocol_header;
  } payload;
} amqp_frame_t;

typedef enum amqp_response_type_enum_ {
  AMQP_RESPONSE_NONE = 0,
  AMQP_RESPONSE_NORMAL,
  AMQP_RESPONSE_LIBRARY_EXCEPTION,
  AMQP_RESPONSE_SERVER_EXCEPTION
} amqp_response_type_enum;

typedef struct amqp_rpc_reply_t_ {
  amqp_response_type_enum reply_type;
  amqp_method_t reply;
  int library_error; /* if AMQP_RESPONSE_LIBRARY_EXCEPTION, then 0 here means socket EOF */
} amqp_rpc_reply_t;

typedef enum amqp_sasl_method_enum_ {
  AMQP_SASL_METHOD_PLAIN = 0
} amqp_sasl_method_enum;

/* Opaque struct. */
typedef struct amqp_connection_state_t_ *amqp_connection_state_t;

AMQP_PUBLIC_FUNCTION
char const *
AMQP_CALL amqp_version(void);

/* Exported empty data structures */
AMQP_PUBLIC_VARIABLE const amqp_bytes_t amqp_empty_bytes;
AMQP_PUBLIC_VARIABLE const amqp_table_t amqp_empty_table;
AMQP_PUBLIC_VARIABLE const amqp_array_t amqp_empty_array;

/* Compatibility macros for the above, to avoid the need to update
   code written against earlier versions of librabbitmq. */
#define AMQP_EMPTY_BYTES amqp_empty_bytes
#define AMQP_EMPTY_TABLE amqp_empty_table
#define AMQP_EMPTY_ARRAY amqp_empty_array

AMQP_PUBLIC_FUNCTION
void
AMQP_CALL init_amqp_pool(amqp_pool_t *pool, size_t pagesize);

AMQP_PUBLIC_FUNCTION
void
AMQP_CALL recycle_amqp_pool(amqp_pool_t *pool);

AMQP_PUBLIC_FUNCTION
void
AMQP_CALL empty_amqp_pool(amqp_pool_t *pool);

AMQP_PUBLIC_FUNCTION
void *
AMQP_CALL amqp_pool_alloc(amqp_pool_t *pool, size_t amount);

AMQP_PUBLIC_FUNCTION
void
AMQP_CALL amqp_pool_alloc_bytes(amqp_pool_t *pool, size_t amount, amqp_bytes_t *output);

AMQP_PUBLIC_FUNCTION
amqp_bytes_t
AMQP_CALL amqp_cstring_bytes(char const *cstr);

AMQP_PUBLIC_FUNCTION
amqp_bytes_t
AMQP_CALL amqp_bytes_malloc_dup(amqp_bytes_t src);

AMQP_PUBLIC_FUNCTION
amqp_bytes_t
AMQP_CALL amqp_bytes_malloc(size_t amount);

AMQP_PUBLIC_FUNCTION
void
AMQP_CALL amqp_bytes_free(amqp_bytes_t bytes);

AMQP_PUBLIC_FUNCTION
amqp_connection_state_t
AMQP_CALL amqp_new_connection(void);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_get_sockfd(amqp_connection_state_t state);

AMQP_PUBLIC_FUNCTION
void
AMQP_CALL amqp_set_sockfd(amqp_connection_state_t state, int sockfd);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_tune_connection(amqp_connection_state_t state,
            int channel_max,
            int frame_max,
            int heartbeat);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_get_channel_max(amqp_connection_state_t state);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_destroy_connection(amqp_connection_state_t state);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_handle_input(amqp_connection_state_t state,
		        amqp_bytes_t received_data,
		        amqp_frame_t *decoded_frame);

AMQP_PUBLIC_FUNCTION
amqp_boolean_t
AMQP_CALL amqp_release_buffers_ok(amqp_connection_state_t state);

AMQP_PUBLIC_FUNCTION
void
AMQP_CALL amqp_release_buffers(amqp_connection_state_t state);

AMQP_PUBLIC_FUNCTION
void
AMQP_CALL amqp_maybe_release_buffers(amqp_connection_state_t state);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_send_frame(amqp_connection_state_t state, amqp_frame_t const *frame);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_table_entry_cmp(void const *entry1, void const *entry2);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_open_socket(char const *hostname, int portnumber);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_send_header(amqp_connection_state_t state);

AMQP_PUBLIC_FUNCTION
amqp_boolean_t
AMQP_CALL amqp_frames_enqueued(amqp_connection_state_t state);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_simple_wait_frame(amqp_connection_state_t state,
		       amqp_frame_t *decoded_frame);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_simple_wait_method(amqp_connection_state_t state,
			      amqp_channel_t expected_channel,
			      amqp_method_number_t expected_method,
			      amqp_method_t *output);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_send_method(amqp_connection_state_t state,
		        amqp_channel_t channel,
		        amqp_method_number_t id,
		        void *decoded);

AMQP_PUBLIC_FUNCTION
amqp_rpc_reply_t
AMQP_CALL amqp_simple_rpc(amqp_connection_state_t state,
		        amqp_channel_t channel,
		        amqp_method_number_t request_id,
		        amqp_method_number_t *expected_reply_ids,
		        void *decoded_request_method);

AMQP_PUBLIC_FUNCTION
void *
AMQP_CALL amqp_simple_rpc_decoded(amqp_connection_state_t state,
		        amqp_channel_t channel,
			      amqp_method_number_t request_id,
			      amqp_method_number_t reply_id,
			      void *decoded_request_method);

/*
 * The API methods corresponding to most synchronous AMQP methods
 * return a pointer to the decoded method result.  Upon error, they
 * return NULL, and we need some way of discovering what, if anything,
 * went wrong. amqp_get_rpc_reply() returns the most recent
 * amqp_rpc_reply_t instance corresponding to such an API operation
 * for the given connection.
 *
 * Only use it for operations that do not themselves return
 * amqp_rpc_reply_t; operations that do return amqp_rpc_reply_t
 * generally do NOT update this per-connection-global amqp_rpc_reply_t
 * instance.
 */
AMQP_PUBLIC_FUNCTION
amqp_rpc_reply_t
AMQP_CALL amqp_get_rpc_reply(amqp_connection_state_t state);

AMQP_PUBLIC_FUNCTION
amqp_rpc_reply_t
AMQP_CALL amqp_login(amqp_connection_state_t state, char const *vhost,
            int channel_max, int frame_max, int heartbeat,
	          amqp_sasl_method_enum sasl_method, ...);

struct amqp_basic_properties_t_;

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_basic_publish(amqp_connection_state_t state, amqp_channel_t channel,
            amqp_bytes_t exchange, amqp_bytes_t routing_key,
		        amqp_boolean_t mandatory, amqp_boolean_t immediate,
		        struct amqp_basic_properties_t_ const *properties,
		        amqp_bytes_t body);

AMQP_PUBLIC_FUNCTION
amqp_rpc_reply_t
AMQP_CALL amqp_channel_close(amqp_connection_state_t state, amqp_channel_t channel,
		        int code);

AMQP_PUBLIC_FUNCTION
amqp_rpc_reply_t
AMQP_CALL amqp_connection_close(amqp_connection_state_t state, int code);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_basic_ack(amqp_connection_state_t state, amqp_channel_t channel,
	          uint64_t delivery_tag, amqp_boolean_t multiple);

AMQP_PUBLIC_FUNCTION
amqp_rpc_reply_t
AMQP_CALL amqp_basic_get(amqp_connection_state_t state, amqp_channel_t channel,
	          amqp_bytes_t queue, amqp_boolean_t no_ack);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_basic_reject(amqp_connection_state_t state, amqp_channel_t channel,
		        uint64_t delivery_tag, amqp_boolean_t requeue);

/*
 * Can be used to see if there is data still in the buffer, if so
 * calling amqp_simple_wait_frame will not immediately enter a
 * blocking read.
 *
 * Possibly amqp_frames_enqueued should be used for this?
 */
AMQP_PUBLIC_FUNCTION
amqp_boolean_t
AMQP_CALL amqp_data_in_buffer(amqp_connection_state_t state);

/*
 * Get the error string for the given error code.
 *
 * The returned string resides on the heap; the caller is responsible
 * for freeing it.
 */
AMQP_PUBLIC_FUNCTION
char *
AMQP_CALL amqp_error_string(int err);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_decode_table(amqp_bytes_t encoded, amqp_pool_t *pool,
		        amqp_table_t *output, size_t *offset);

AMQP_PUBLIC_FUNCTION
int
AMQP_CALL amqp_encode_table(amqp_bytes_t encoded, amqp_table_t *input, size_t *offset);

struct amqp_connection_info {
  char *user;
  char *password;
  char *host;
  char *vhost;
  int port;
};

AMQP_PUBLIC_FUNCTION
void 
AMQP_CALL amqp_default_connection_info(struct amqp_connection_info *parsed);

AMQP_PUBLIC_FUNCTION
int 
AMQP_CALL amqp_parse_url(char *url, struct amqp_connection_info *parsed);

AMQP_END_DECLS

#include <amqp_framing.h>

#endif /* AMQP_H */
