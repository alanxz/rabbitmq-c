// Copyright 2007 - 2022, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>

// Drives amqp_decode_method(), the generated decoder for every AMQP method
// frame. The first 4 bytes are the method id (big-endian), the rest the body.
extern int LLVMFuzzerTestOneInput(const char *data, size_t size) {

  int unused_result;
  amqp_pool_t pool;
  amqp_method_number_t method_id;

  if (size < 4) {
    return 0;
  }

  method_id = ((amqp_method_number_t)(uint8_t)data[0] << 24) |
              ((amqp_method_number_t)(uint8_t)data[1] << 16) |
              ((amqp_method_number_t)(uint8_t)data[2] << 8) |
              ((amqp_method_number_t)(uint8_t)data[3]);

  init_amqp_pool(&pool, 4096);
  {
    void *decoded = NULL;
    amqp_bytes_t encoded;
    encoded.len = size - 4;
    encoded.bytes = (void *)(data + 4);

    unused_result = amqp_decode_method(method_id, &pool, encoded, &decoded);
  }
  empty_amqp_pool(&pool);
  return 0;
}
