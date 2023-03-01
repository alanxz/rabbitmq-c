// Copyright 2007 - 2022, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>

extern int LLVMFuzzerTestOneInput(const char *data, size_t size) {
  // amqp_parse_url expects null-terminated string that it can modify,
  // LLVMFuzzer expects that data will not be modified and won't necessarily
  // null terminate the string, so do that here.
  char* in = malloc(size + 1);
  memcpy(in, data, size);
  in[size] = '\0';

  struct amqp_connection_info ci;
  amqp_parse_url(in, &ci);
  free(in);
  return 0;
}
