#ifndef librabbitmq_amqp_h
#define librabbitmq_amqp_h

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

typedef struct amqp_table_entry_t_ {
  amqp_bytes_t key;
  char kind;
  union {
    amqp_bytes_t bytes;
    int32_t i32;
    amqp_decimal_t decimal;
    uint64_t u64;
    amqp_table_t table;
  } value;
} amqp_table_entry_t;

#define _AMQP_TE_INIT(ke,ki,v) { .key = (ke), .kind = (ki), .value = { v } }
#define AMQP_TABLE_ENTRY_S(k,v) _AMQP_TE_INIT(amqp_cstring_bytes(k), 'S', .bytes = (v))
#define AMQP_TABLE_ENTRY_I(k,v) _AMQP_TE_INIT(amqp_cstring_bytes(k), 'I', .i32 = (v))
#define AMQP_TABLE_ENTRY_D(k,v) _AMQP_TE_INIT(amqp_cstring_bytes(k), 'D', .decimal = (v))
#define AMQP_TABLE_ENTRY_T(k,v) _AMQP_TE_INIT(amqp_cstring_bytes(k), 'T', .u64 = (v))
#define AMQP_TABLE_ENTRY_F(k,v) _AMQP_TE_INIT(amqp_cstring_bytes(k), 'F', .table = (v))

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
  int library_errno; /* if AMQP_RESPONSE_LIBRARY_EXCEPTION, then 0 here means socket EOF */
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

extern amqp_rpc_reply_t amqp_rpc_reply;

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

/*
 * Can be used to see if there is data still in the buffer, if so
 * calling amqp_simple_wait_frame will not immediately enter a
 * blocking read.
 *
 * Possibly amqp_frames_enqueued should be used for this?
 */
extern amqp_boolean_t amqp_data_in_buffer(amqp_connection_state_t state);

/*
 * Expose amqp_rpc_reply to libraries.
 */
extern amqp_rpc_reply_t amqp_get_rpc_reply(void);

#ifdef __cplusplus
}
#endif

#endif
