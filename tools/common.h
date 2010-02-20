#include <stdint.h>

#include <amqp.h>
#include <amqp_framing.h>

extern char *amqp_server_exception_string(amqp_rpc_reply_t r);
extern char *amqp_rpc_reply_string(amqp_rpc_reply_t r);

extern void die(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));
extern void die_errno(int err, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));
extern void die_rpc(amqp_rpc_reply_t r, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

extern const char *connect_options_title;
extern struct poptOption connect_options[];
extern amqp_connection_state_t make_connection(void);
extern void close_connection(amqp_connection_state_t conn);

extern amqp_bytes_t read_all(int fd);
extern void write_all(int fd, amqp_bytes_t data);

extern void copy_body(amqp_connection_state_t conn, int fd);

struct pipeline {
	int pid;
	int infd;
};

extern void pipeline(const char * const *argv, struct pipeline *pl);
extern int finish_pipeline(struct pipeline *pl);

#define INCLUDE_OPTIONS(options) \
	{NULL, 0, POPT_ARG_INCLUDE_TABLE, options, 0, options ## _title, NULL}

extern poptContext process_options(int argc, const char **argv,
				   struct poptOption *options,
				   const char *help);
extern void process_all_options(int argc, const char **argv,
				struct poptOption *options);

extern amqp_bytes_t cstring_bytes(const char *str);
