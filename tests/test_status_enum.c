// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rabbitmq-c/amqp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check_errorstrings(amqp_status_enum start, amqp_status_enum end) {
  int i;
  for (i = start; i > end; --i) {
    const char* err = amqp_error_string2(i);
    if (0 == strcmp(err, "(unknown error)")) {
      printf("amqp_status_enum value %s%X", i < 0 ? "-" : "", (unsigned)i);
      abort();
    }
  }
}

int main(void) {
  check_errorstrings(AMQP_STATUS_OK, _AMQP_STATUS_NEXT_VALUE);
  check_errorstrings(AMQP_STATUS_TCP_ERROR, _AMQP_STATUS_TCP_NEXT_VALUE);
  check_errorstrings(AMQP_STATUS_SSL_ERROR, _AMQP_STATUS_SSL_NEXT_VALUE);

  return 0;
}
