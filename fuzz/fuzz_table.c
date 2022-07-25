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

extern int LLVMFuzzerTestOneInput(const char *data, size_t size) {

  int result;
  amqp_pool_t pool;

  init_amqp_pool(&pool, 4096);
  {
    amqp_table_t decoded;
    size_t decoding_offset = 0;
    amqp_bytes_t decoding_bytes;
    decoding_bytes.len = size;
    decoding_bytes.bytes = (uint8_t *)data;

    result =
        amqp_decode_table(decoding_bytes, &pool, &decoded, &decoding_offset);
  }
  return result;
}
