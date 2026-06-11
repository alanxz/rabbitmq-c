// Copyright 2007 - 2026, Arthur Chan and the rabbitmq-c contributors.
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

// Drives amqp_decode_properties(), the generated decoder for every AMQP
// content-header (properties) frame. The first 2 bytes are the class id
// (big-endian), the rest the body.
extern int LLVMFuzzerTestOneInput(const char *data, size_t size) {

  int unused_result;
  amqp_pool_t pool;
  uint16_t class_id;

  if (size < 2) {
    return 0;
  }

  class_id = ((uint16_t)(uint8_t)data[0] << 8) |
             ((uint16_t)(uint8_t)data[1]);

  init_amqp_pool(&pool, 4096);
  {
    void *decoded = NULL;
    amqp_bytes_t encoded;
    encoded.len = size - 2;
    encoded.bytes = (void *)(data + 2);

    unused_result = amqp_decode_properties(class_id, &pool, encoded, &decoded);
  }
  empty_amqp_pool(&pool);
  return 0;
}
