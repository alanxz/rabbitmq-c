// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <stdint.h>

#include <popt.h>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>

extern const char *amqp_server_exception_string(amqp_rpc_reply_t r);
extern const char *amqp_rpc_reply_string(amqp_rpc_reply_t r);

extern void die(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
extern void die_errno(int err, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
extern void die_amqp_error(int err, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
extern void die_rpc(amqp_rpc_reply_t r, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

extern const char *connect_options_title;
extern struct poptOption connect_options[];
extern amqp_connection_state_t make_connection(void);
extern void close_connection(amqp_connection_state_t conn);

extern amqp_bytes_t read_all(int fd);
extern void write_all(int fd, amqp_bytes_t data);

extern void copy_body(amqp_connection_state_t conn, int fd);

#define INCLUDE_OPTIONS(options) \
  { NULL, 0, POPT_ARG_INCLUDE_TABLE, options, 0, options##_title, NULL }

extern poptContext process_options(int argc, const char **argv,
                                   struct poptOption *options,
                                   const char *help);
extern void process_all_options(int argc, const char **argv,
                                struct poptOption *options);

extern amqp_bytes_t cstring_bytes(const char *str);
