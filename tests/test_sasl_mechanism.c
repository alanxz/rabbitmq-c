// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <amqp_socket.h>

static void parse_success(amqp_bytes_t mechanisms,
                          amqp_sasl_method_enum method) {
  if (!sasl_mechanism_in_list(mechanisms, method)) {
    fprintf(stderr, "Expected to find mechanism in list, but didn't: %s\n",
            (char *)mechanisms.bytes);
    abort();
  }
}

static void parse_fail(amqp_bytes_t mechanisms, amqp_sasl_method_enum method) {
  if (sasl_mechanism_in_list(mechanisms, method)) {
    fprintf(stderr,
            "Expected the mechanism not on the list, but it was present: %s\n",
            (char *)mechanisms.bytes);
    abort();
  }
}

int main(void) {
  parse_success(amqp_cstring_bytes("DIGEST-MD5 CRAM-MD5 LOGIN PLAIN"),
                AMQP_SASL_METHOD_PLAIN);
  parse_fail(amqp_cstring_bytes("DIGEST-MD5 CRAM-MD5 LOGIN PLAIN"),
             AMQP_SASL_METHOD_EXTERNAL);
  parse_success(amqp_cstring_bytes("DIGEST-MD5 CRAM-MD5 EXTERNAL"),
                AMQP_SASL_METHOD_EXTERNAL);
  parse_fail(amqp_cstring_bytes("DIGEST-MD5 CRAM-MD5 EXTERNAL"),
             AMQP_SASL_METHOD_PLAIN);
  return 0;
}
