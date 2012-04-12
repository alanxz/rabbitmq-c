#ifndef librabbitmq_amqp_h
#define librabbitmq_amqp_h

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

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef  _WIN32
#ifdef BUILDING_LIBRABBITMQ
#define RABBITMQ_EXPORT extern __declspec(dllexport)
#else
#define RABBITMQ_EXPORT extern __declspec(dllimport)
#endif
#else
#define RABBITMQ_EXPORT extern
#endif

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

RABBITMQ_EXPORT char const *amqp_version(void);

/* Exported empty data structures */
RABBITMQ_EXPORT const amqp_bytes_t amqp_empty_bytes;
RABBITMQ_EXPORT const amqp_table_t amqp_empty_table;
RABBITMQ_EXPORT const amqp_array_t amqp_empty_array;

/* Compatibility macros for the above, to avoid the need to update
   code written against earlier versions of librabbitmq. */
#define AMQP_EMPTY_BYTES amqp_empty_bytes
#define AMQP_EMPTY_TABLE amqp_empty_table
#define AMQP_EMPTY_ARRAY amqp_empty_array

RABBITMQ_EXPORT void init_amqp_pool(amqp_pool_t *pool, size_t pagesize);
RABBITMQ_EXPORT void recycle_amqp_pool(amqp_pool_t *pool);
RABBITMQ_EXPORT void empty_amqp_pool(amqp_pool_t *pool);

RABBITMQ_EXPORT void *amqp_pool_alloc(amqp_pool_t *pool, size_t amount);
RABBITMQ_EXPORT void amqp_pool_alloc_bytes(amqp_pool_t *pool,
                                          size_t amount, amqp_bytes_t *output);

RABBITMQ_EXPORT amqp_bytes_t amqp_cstring_bytes(char const *cstr);
RABBITMQ_EXPORT amqp_bytes_t amqp_bytes_malloc_dup(amqp_bytes_t src);
RABBITMQ_EXPORT amqp_bytes_t amqp_bytes_malloc(size_t amount);
RABBITMQ_EXPORT void amqp_bytes_free(amqp_bytes_t bytes);

RABBITMQ_EXPORT amqp_connection_state_t amqp_new_connection(void);
RABBITMQ_EXPORT int amqp_get_sockfd(amqp_connection_state_t state);
RABBITMQ_EXPORT void amqp_set_sockfd(amqp_connection_state_t state,
				     int sockfd);
RABBITMQ_EXPORT int amqp_tune_connection(amqp_connection_state_t state,
					 int channel_max,
					 int frame_max,
					 int heartbeat);
RABBITMQ_EXPORT int amqp_get_channel_max(amqp_connection_state_t state);
RABBITMQ_EXPORT int amqp_destroy_connection(amqp_connection_state_t state);

RABBITMQ_EXPORT int amqp_handle_input(amqp_connection_state_t state,
				      amqp_bytes_t received_data,
				      amqp_frame_t *decoded_frame);

RABBITMQ_EXPORT amqp_boolean_t amqp_release_buffers_ok(
                                                amqp_connection_state_t state);

RABBITMQ_EXPORT void amqp_release_buffers(amqp_connection_state_t state);

RABBITMQ_EXPORT void amqp_maybe_release_buffers(amqp_connection_state_t state);

RABBITMQ_EXPORT int amqp_send_frame(amqp_connection_state_t state,
				    amqp_frame_t const *frame);

RABBITMQ_EXPORT int amqp_table_entry_cmp(void const *entry1,
					 void const *entry2);

RABBITMQ_EXPORT int amqp_open_socket(char const *hostname,
				     int portnumber);

RABBITMQ_EXPORT int amqp_send_header(amqp_connection_state_t state);

RABBITMQ_EXPORT amqp_boolean_t amqp_frames_enqueued(
                                               amqp_connection_state_t state);

RABBITMQ_EXPORT int amqp_simple_wait_frame(amqp_connection_state_t state,
					   amqp_frame_t *decoded_frame);

RABBITMQ_EXPORT int amqp_simple_wait_method(amqp_connection_state_t state,
                                          amqp_channel_t expected_channel,
                                          amqp_method_number_t expected_method,
                                          amqp_method_t *output);

RABBITMQ_EXPORT int amqp_send_method(amqp_connection_state_t state,
				     amqp_channel_t channel,
				     amqp_method_number_t id,
				     void *decoded);

RABBITMQ_EXPORT amqp_rpc_reply_t amqp_simple_rpc(amqp_connection_state_t state,
                                      amqp_channel_t channel,
                                      amqp_method_number_t request_id,
                                      amqp_method_number_t *expected_reply_ids,
                                      void *decoded_request_method);

RABBITMQ_EXPORT void *amqp_simple_rpc_decoded(amqp_connection_state_t state,
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
RABBITMQ_EXPORT amqp_rpc_reply_t amqp_get_rpc_reply(
                                                amqp_connection_state_t state);

RABBITMQ_EXPORT amqp_rpc_reply_t amqp_login(amqp_connection_state_t state,
                                        char const *vhost,
                                        int channel_max,
                                        int frame_max,
                                        int heartbeat,
                                        amqp_sasl_method_enum sasl_method, ...);

struct amqp_basic_properties_t_;
RABBITMQ_EXPORT int amqp_basic_publish(amqp_connection_state_t state,
                              amqp_channel_t channel,
                              amqp_bytes_t exchange,
                              amqp_bytes_t routing_key,
                              amqp_boolean_t mandatory,
                              amqp_boolean_t immediate,
                              struct amqp_basic_properties_t_ const *properties,
                              amqp_bytes_t body);

RABBITMQ_EXPORT amqp_rpc_reply_t amqp_channel_close(
                                                 amqp_connection_state_t state,
                                                 amqp_channel_t channel,
                                                 int code);
RABBITMQ_EXPORT amqp_rpc_reply_t amqp_connection_close(
                                                 amqp_connection_state_t state,
                                                 int code);

RABBITMQ_EXPORT int amqp_basic_ack(amqp_connection_state_t state,
				   amqp_channel_t channel,
				   uint64_t delivery_tag,
				   amqp_boolean_t multiple);

RABBITMQ_EXPORT amqp_rpc_reply_t amqp_basic_get(amqp_connection_state_t state,
                                                amqp_channel_t channel,
                                                amqp_bytes_t queue,
                                                amqp_boolean_t no_ack);

RABBITMQ_EXPORT int amqp_basic_reject(amqp_connection_state_t state,
				      amqp_channel_t channel,
				      uint64_t delivery_tag,
				      amqp_boolean_t requeue);

/*
 * Can be used to see if there is data still in the buffer, if so
 * calling amqp_simple_wait_frame will not immediately enter a
 * blocking read.
 *
 * Possibly amqp_frames_enqueued should be used for this?
 */
RABBITMQ_EXPORT amqp_boolean_t amqp_data_in_buffer(
                                                amqp_connection_state_t state);

/*
 * Get the error string for the given error code.
 *
 * The returned string resides on the heap; the caller is responsible
 * for freeing it.
 */
RABBITMQ_EXPORT char *amqp_error_string(int err);

RABBITMQ_EXPORT int amqp_decode_table(amqp_bytes_t encoded,
                                      amqp_pool_t *pool,
                                      amqp_table_t *output,
                                      size_t *offset);

RABBITMQ_EXPORT int amqp_encode_table(amqp_bytes_t encoded,
                                      amqp_table_t *input,
                                      size_t *offset);

struct amqp_connection_info {
  char *user;
  char *password;
  char *host;
  char *vhost;
  int port;
};

RABBITMQ_EXPORT void amqp_default_connection_info(
					  struct amqp_connection_info *parsed);
RABBITMQ_EXPORT int amqp_parse_url(char *url,
				   struct amqp_connection_info *parsed);

#ifdef __cplusplus
}
#endif

#include <amqp_framing.h>

#endif
