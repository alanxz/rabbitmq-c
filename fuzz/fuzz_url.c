// Copyright 2007 - 2022, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>

extern int LLVMFuzzerTestOneInput(const char *data, size_t size) {

  struct amqp_connection_info ci;
  int res;
  res = amqp_parse_url((char *)data, &ci);
  return res;
}
