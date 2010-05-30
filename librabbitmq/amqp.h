#ifndef librabbitmq_amqp_h
#define librabbitmq_amqp_h

/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * The Original Code is librabbitmq.
 *
 * The Initial Developers of the Original Code are LShift Ltd, Cohesive
 * Financial Technologies LLC, and Rabbit Technologies Ltd.  Portions
 * created before 22-Nov-2008 00:00:00 GMT by LShift Ltd, Cohesive
 * Financial Technologies LLC, or Rabbit Technologies Ltd are Copyright
 * (C) 2007-2008 LShift Ltd, Cohesive Financial Technologies LLC, and
 * Rabbit Technologies Ltd.
 *
 * Portions created by LShift Ltd are Copyright (C) 2007-2009 LShift
 * Ltd. Portions created by Cohesive Financial Technologies LLC are
 * Copyright (C) 2007-2009 Cohesive Financial Technologies
 * LLC. Portions created by Rabbit Technologies Ltd are Copyright (C)
 * 2007-2009 Rabbit Technologies Ltd.
 *
 * Portions created by Tony Garnock-Jones are Copyright (C) 2009-2010
 * LShift Ltd and Tony Garnock-Jones.
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
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

#ifdef __cplusplus
extern "C" {
#endif

typedef int amqp_boolean_t;
typedef uint32_t amqp_method_number_t;
typedef uint32_t amqp_flags_t;
typedef uint16_t amqp_channel_t;

typedef struct amqp_bytes_t_ {
  size_t len;
  void *bytes;
} amqp_bytes_t;

#define AMQP_EMPTY_BYTES ((amqp_bytes_t) { .len = 0, .bytes = NULL })

typedef struct amqp_decimal_t_ {
  int decimals;
  uint32_t value;
} amqp_decimal_t;

#define AMQP_DECIMAL(d,v) ((amqp_decimal_t) { .decimals = (d), .value = (v) })

typedef struct amqp_table_t_ {
  int num_entries;
  struct amqp_table_entry_t_ *entries;
} amqp_table_t;

#define AMQP_EMPTY_TABLE ((amqp_table_t) { .num_entries = 0, .entries = NULL })

typedef struct amqp_array_t_ {
  int num_entries;
  struct amqp_field_value_t_ *entries;
} amqp_array_t;

#define AMQP_EMPTY_ARRAY ((amqp_array_t) { .num_entries = 0, .entries = NULL })

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
  char kind;
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
  AMQP_FIELD_KIND_F32 = 'f',
  AMQP_FIELD_KIND_F64 = 'd',
  AMQP_FIELD_KIND_DECIMAL = 'D',
  AMQP_FIELD_KIND_UTF8 = 'S',
  AMQP_FIELD_KIND_ARRAY = 'A',
  AMQP_FIELD_KIND_TIMESTAMP = 'T',
  AMQP_FIELD_KIND_TABLE = 'F',
  AMQP_FIELD_KIND_VOID = 'V',
  AMQP_FIELD_KIND_BYTES = 'x',
} amqp_field_value_kind_t;

#define _AMQP_TEINIT(ke,ki,v) {.key = (ke), .value = {.kind = AMQP_FIELD_KIND_##ki, .value = {v}}}
#define AMQP_TABLE_ENTRY_BOOLEAN(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), BOOLEAN, .boolean = (v))
#define AMQP_TABLE_ENTRY_I8(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), I8, .i8 = (v))
#define AMQP_TABLE_ENTRY_U8(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), U8, .u8 = (v))
#define AMQP_TABLE_ENTRY_I16(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), I16, .i16 = (v))
#define AMQP_TABLE_ENTRY_U16(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), U16, .u16 = (v))
#define AMQP_TABLE_ENTRY_I32(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), I32, .i32 = (v))
#define AMQP_TABLE_ENTRY_U32(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), U32, .u32 = (v))
#define AMQP_TABLE_ENTRY_I64(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), I64, .i64 = (v))
#define AMQP_TABLE_ENTRY_F32(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), F32, .f32 = (v))
#define AMQP_TABLE_ENTRY_F64(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), F64, .f64 = (v))
#define AMQP_TABLE_ENTRY_DECIMAL(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), DECIMAL, .decimal = (v))
#define AMQP_TABLE_ENTRY_UTF8(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), UTF8, .bytes = (v))
#define AMQP_TABLE_ENTRY_ARRAY(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), ARRAY, .array = (v))
#define AMQP_TABLE_ENTRY_TIMESTAMP(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), TIMESTAMP, .u64 = (v))
#define AMQP_TABLE_ENTRY_TABLE(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), TABLE, .table = (v))
#define AMQP_TABLE_ENTRY_VOID(k)   _AMQP_TEINIT(amqp_cstring_bytes(k), VOID, .u8 = 0)
#define AMQP_TABLE_ENTRY_BYTES(k,v) _AMQP_TEINIT(amqp_cstring_bytes(k), BYTES, .bytes = (v))

#define _AMQP_FVINIT(ki,v) {.kind = AMQP_FIELD_KIND_##ki, .value = {v}}
#define AMQP_FIELD_VALUE_BOOLEAN(v) _AMQP_FVINIT(BOOLEAN, .boolean = (v))
#define AMQP_FIELD_VALUE_I8(v) _AMQP_FVINIT(I8, .i8 = (v))
#define AMQP_FIELD_VALUE_U8(v) _AMQP_FVINIT(U8, .u8 = (v))
#define AMQP_FIELD_VALUE_I16(v) _AMQP_FVINIT(I16, .i16 = (v))
#define AMQP_FIELD_VALUE_U16(v) _AMQP_FVINIT(U16, .u16 = (v))
#define AMQP_FIELD_VALUE_I32(v) _AMQP_FVINIT(I32, .i32 = (v))
#define AMQP_FIELD_VALUE_U32(v) _AMQP_FVINIT(U32, .u32 = (v))
#define AMQP_FIELD_VALUE_I64(v) _AMQP_FVINIT(I64, .i64 = (v))
#define AMQP_FIELD_VALUE_F32(v) _AMQP_FVINIT(F32, .f32 = (v))
#define AMQP_FIELD_VALUE_F64(v) _AMQP_FVINIT(F64, .f64 = (v))
#define AMQP_FIELD_VALUE_DECIMAL(v) _AMQP_FVINIT(DECIMAL, .decimal = (v))
#define AMQP_FIELD_VALUE_UTF8(v) _AMQP_FVINIT(UTF8, .bytes = (v))
#define AMQP_FIELD_VALUE_ARRAY(v) _AMQP_FVINIT(ARRAY, .array = (v))
#define AMQP_FIELD_VALUE_TIMESTAMP(v) _AMQP_FVINIT(TIMESTAMP, .u64 = (v))
#define AMQP_FIELD_VALUE_TABLE(v) _AMQP_FVINIT(TABLE, .table = (v))
#define AMQP_FIELD_VALUE_VOID(k)   _AMQP_FVINIT(VOID, .u8 = 0)
#define AMQP_FIELD_VALUE_BYTES(v) _AMQP_FVINIT(BYTES, .bytes = (v))

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

#define AMQP_PSEUDOFRAME_PROTOCOL_HEADER ((uint8_t) 'A')
#define AMQP_PSEUDOFRAME_PROTOCOL_CHANNEL ((amqp_channel_t) ((((int) 'M') << 8) | ((int) 'Q')))

typedef int (*amqp_output_fn_t)(void *context, void *buffer, size_t count);

/* Opaque struct. */
typedef struct amqp_connection_state_t_ *amqp_connection_state_t;

extern char const *amqp_version(void);

extern void init_amqp_pool(amqp_pool_t *pool, size_t pagesize);
extern void recycle_amqp_pool(amqp_pool_t *pool);
extern void empty_amqp_pool(amqp_pool_t *pool);

extern void *amqp_pool_alloc(amqp_pool_t *pool, size_t amount);
extern void amqp_pool_alloc_bytes(amqp_pool_t *pool, size_t amount, amqp_bytes_t *output);

extern amqp_bytes_t amqp_cstring_bytes(char const *cstr);
extern amqp_bytes_t amqp_bytes_malloc_dup(amqp_bytes_t src);
extern amqp_bytes_t amqp_bytes_malloc(size_t amount);
extern void amqp_bytes_free(amqp_bytes_t bytes);

#define AMQP_BYTES_FREE(b)			\
  ({						\
    if ((b).bytes != NULL) {			\
      free((b).bytes);				\
      (b).bytes = NULL;				\
    }						\
  })

extern amqp_connection_state_t amqp_new_connection(void);
extern int amqp_get_sockfd(amqp_connection_state_t state);
extern void amqp_set_sockfd(amqp_connection_state_t state,
			    int sockfd);
extern int amqp_tune_connection(amqp_connection_state_t state,
				int channel_max,
				int frame_max,
				int heartbeat);
int amqp_get_channel_max(amqp_connection_state_t state);
extern void amqp_destroy_connection(amqp_connection_state_t state);
extern int amqp_end_connection(amqp_connection_state_t state);

extern int amqp_handle_input(amqp_connection_state_t state,
			     amqp_bytes_t received_data,
			     amqp_frame_t *decoded_frame);

extern amqp_boolean_t amqp_release_buffers_ok(amqp_connection_state_t state);

extern void amqp_release_buffers(amqp_connection_state_t state);

extern void amqp_maybe_release_buffers(amqp_connection_state_t state);

extern int amqp_send_frame(amqp_connection_state_t state,
			   amqp_frame_t const *frame);
extern int amqp_send_frame_to(amqp_connection_state_t state,
			      amqp_frame_t const *frame,
			      amqp_output_fn_t fn,
			      void *context);

extern int amqp_table_entry_cmp(void const *entry1, void const *entry2);

extern int amqp_open_socket(char const *hostname, int portnumber);

extern int amqp_send_header(amqp_connection_state_t state);
extern int amqp_send_header_to(amqp_connection_state_t state,
			       amqp_output_fn_t fn,
			       void *context);

extern amqp_boolean_t amqp_frames_enqueued(amqp_connection_state_t state);

extern int amqp_simple_wait_frame(amqp_connection_state_t state,
				  amqp_frame_t *decoded_frame);

extern int amqp_simple_wait_method(amqp_connection_state_t state,
				   amqp_channel_t expected_channel,
				   amqp_method_number_t expected_method,
				   amqp_method_t *output);

extern int amqp_send_method(amqp_connection_state_t state,
			    amqp_channel_t channel,
			    amqp_method_number_t id,
			    void *decoded);

extern amqp_rpc_reply_t amqp_simple_rpc(amqp_connection_state_t state,
					amqp_channel_t channel,
					amqp_method_number_t request_id,
					amqp_method_number_t *expected_reply_ids,
					void *decoded_request_method);

#define AMQP_EXPAND_METHOD(classname, methodname) (AMQP_ ## classname ## _ ## methodname ## _METHOD)

#define AMQP_SIMPLE_RPC(state, channel, classname, requestname, replyname, structname, ...) \
  ({									\
    structname _simple_rpc_request___ = (structname) { __VA_ARGS__ };	\
    amqp_method_number_t _replies__[2] = { AMQP_EXPAND_METHOD(classname, replyname), 0}; \
    amqp_simple_rpc(state, channel,					\
		    AMQP_EXPAND_METHOD(classname, requestname),	\
		    (amqp_method_number_t *)&_replies__,	\
		    &_simple_rpc_request___);				\
  })

#define AMQP_MULTIPLE_RESPONSE_RPC(state, channel, classname, requestname, replynames, structname, ...) \
  ({									\
    structname _simple_rpc_request___ = (structname) { __VA_ARGS__ };	\
    amqp_simple_rpc(state, channel,					\
		    AMQP_EXPAND_METHOD(classname, requestname),	\
		    replynames,	\
		    &_simple_rpc_request___);				\
  })


extern amqp_rpc_reply_t amqp_login(amqp_connection_state_t state,
				   char const *vhost,
				   int channel_max,
				   int frame_max,
				   int heartbeat,
				   amqp_sasl_method_enum sasl_method, ...);

extern struct amqp_channel_open_ok_t_ *amqp_channel_open(amqp_connection_state_t state,
							 amqp_channel_t channel);

struct amqp_basic_properties_t_;
extern int amqp_basic_publish(amqp_connection_state_t state,
			      amqp_channel_t channel,
			      amqp_bytes_t exchange,
			      amqp_bytes_t routing_key,
			      amqp_boolean_t mandatory,
			      amqp_boolean_t immediate,
			      struct amqp_basic_properties_t_ const *properties,
			      amqp_bytes_t body);

extern amqp_rpc_reply_t amqp_channel_close(amqp_connection_state_t state,
					   amqp_channel_t channel,
					   int code);
extern amqp_rpc_reply_t amqp_connection_close(amqp_connection_state_t state,
					      int code);

extern struct amqp_exchange_declare_ok_t_ *amqp_exchange_declare(amqp_connection_state_t state,
								 amqp_channel_t channel,
								 amqp_bytes_t exchange,
								 amqp_bytes_t type,
								 amqp_boolean_t passive,
								 amqp_boolean_t durable,
								 amqp_boolean_t auto_delete,
								 amqp_table_t arguments);

extern struct amqp_queue_declare_ok_t_ *amqp_queue_declare(amqp_connection_state_t state,
							   amqp_channel_t channel,
							   amqp_bytes_t queue,
							   amqp_boolean_t passive,
							   amqp_boolean_t durable,
							   amqp_boolean_t exclusive,
							   amqp_boolean_t auto_delete,
							   amqp_table_t arguments);

extern struct amqp_queue_bind_ok_t_ *amqp_queue_bind(amqp_connection_state_t state,
						     amqp_channel_t channel,
						     amqp_bytes_t queue,
						     amqp_bytes_t exchange,
						     amqp_bytes_t routing_key,
						     amqp_table_t arguments);

extern struct amqp_queue_unbind_ok_t_ *amqp_queue_unbind(amqp_connection_state_t state,
							 amqp_channel_t channel,
							 amqp_bytes_t queue,
							 amqp_bytes_t exchange,
							 amqp_bytes_t binding_key,
							 amqp_table_t arguments);

extern struct amqp_basic_consume_ok_t_ *amqp_basic_consume(amqp_connection_state_t state,
							   amqp_channel_t channel,
							   amqp_bytes_t queue,
							   amqp_bytes_t consumer_tag,
							   amqp_boolean_t no_local,
							   amqp_boolean_t no_ack,
							   amqp_boolean_t exclusive);

extern int amqp_basic_ack(amqp_connection_state_t state,
			  amqp_channel_t channel,
			  uint64_t delivery_tag,
			  amqp_boolean_t multiple);

extern amqp_rpc_reply_t amqp_basic_get(amqp_connection_state_t state,
          amqp_channel_t channel,
          amqp_bytes_t queue,
          amqp_boolean_t no_ack);

extern struct amqp_queue_purge_ok_t_ *amqp_queue_purge(amqp_connection_state_t state,
            amqp_channel_t channel,
            amqp_bytes_t queue,
            amqp_boolean_t no_wait);

extern struct amqp_tx_select_ok_t_ *amqp_tx_select(amqp_connection_state_t state,
						   amqp_channel_t channel);

extern struct amqp_tx_commit_ok_t_ *amqp_tx_commit(amqp_connection_state_t state,
						   amqp_channel_t channel);

extern struct amqp_tx_rollback_ok_t_ *amqp_tx_rollback(amqp_connection_state_t state,
						       amqp_channel_t channel);

/*
 * Can be used to see if there is data still in the buffer, if so
 * calling amqp_simple_wait_frame will not immediately enter a
 * blocking read.
 *
 * Possibly amqp_frames_enqueued should be used for this?
 */
extern amqp_boolean_t amqp_data_in_buffer(amqp_connection_state_t state);

/*
 * For those API operations (such as amqp_basic_ack,
 * amqp_queue_declare, and so on) that do not themselves return
 * amqp_rpc_reply_t instances, we need some way of discovering what,
 * if anything, went wrong. amqp_get_rpc_reply() returns the most
 * recent amqp_rpc_reply_t instance corresponding to such an API
 * operation for the given connection.
 *
 * Only use it for operations that do not themselves return
 * amqp_rpc_reply_t; operations that do return amqp_rpc_reply_t
 * generally do NOT update this per-connection-global amqp_rpc_reply_t
 * instance.
 */
extern amqp_rpc_reply_t amqp_get_rpc_reply(amqp_connection_state_t state);

/*
 * Get the error string for the given error code.
 *
 * The returned string resides on the heap; the caller is responsible
 * for freeing it.
 */
extern const char *amqp_error_string(int err);

#ifdef __cplusplus
}
#endif

#endif
