// Copyright 2007 - 2026, Arthur Chan and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>

// Round-trips the AMQP codecs. The existing fuzzers only decode; the generated
// serializers (amqp_encode_method/properties/table) and amqp_table_clone are
// unreachable from hand-written input. Each mode decodes attacker bytes into the
// in-memory struct, then re-encodes/clones it -- the decoder supplies the
// adversarial-but-valid structs the encoders would otherwise never see.

#define ENC_BUF_SIZE (1u << 20)

static void roundtrip_method(const uint8_t *p, size_t n) {
  amqp_pool_t pool;
  void *decoded = NULL;
  amqp_method_number_t method_id;
  amqp_bytes_t encoded;

  if (n < 4) {
    return;
  }
  method_id = ((amqp_method_number_t)p[0] << 24) |
              ((amqp_method_number_t)p[1] << 16) |
              ((amqp_method_number_t)p[2] << 8) | (amqp_method_number_t)p[3];
  encoded.len = n - 4;
  encoded.bytes = (void *)(p + 4);

  init_amqp_pool(&pool, 4096);
  if (amqp_decode_method(method_id, &pool, encoded, &decoded) ==
          AMQP_STATUS_OK &&
      decoded != NULL) {
    void *buf = malloc(ENC_BUF_SIZE);
    if (buf != NULL) {
      amqp_bytes_t out;
      out.len = ENC_BUF_SIZE;
      out.bytes = buf;
      amqp_encode_method(method_id, decoded, out);
      free(buf);
    }
  }
  empty_amqp_pool(&pool);
}

static void roundtrip_properties(const uint8_t *p, size_t n) {
  amqp_pool_t pool;
  void *decoded = NULL;
  uint16_t class_id;
  amqp_bytes_t encoded;

  if (n < 2) {
    return;
  }
  class_id = ((uint16_t)p[0] << 8) | (uint16_t)p[1];
  encoded.len = n - 2;
  encoded.bytes = (void *)(p + 2);

  init_amqp_pool(&pool, 4096);
  if (amqp_decode_properties(class_id, &pool, encoded, &decoded) ==
          AMQP_STATUS_OK &&
      decoded != NULL) {
    void *buf = malloc(ENC_BUF_SIZE);
    if (buf != NULL) {
      amqp_bytes_t out;
      out.len = ENC_BUF_SIZE;
      out.bytes = buf;
      amqp_encode_properties(class_id, decoded, out);
      free(buf);
    }
  }
  empty_amqp_pool(&pool);
}

static void roundtrip_table(const uint8_t *p, size_t n) {
  amqp_pool_t pool;
  amqp_table_t decoded;
  amqp_bytes_t encoded;
  size_t offset = 0;

  encoded.len = n;
  encoded.bytes = (void *)p;

  init_amqp_pool(&pool, 4096);
  memset(&decoded, 0, sizeof(decoded));
  if (amqp_decode_table(encoded, &pool, &decoded, &offset) == AMQP_STATUS_OK) {
    void *buf = malloc(ENC_BUF_SIZE);
    if (buf != NULL) {
      amqp_bytes_t out;
      size_t out_off = 0;
      out.len = ENC_BUF_SIZE;
      out.bytes = buf;
      amqp_encode_table(out, &decoded, &out_off);
      free(buf);
    }
    {
      amqp_pool_t clone_pool;
      amqp_table_t clone;
      init_amqp_pool(&clone_pool, 4096);
      memset(&clone, 0, sizeof(clone));
      amqp_table_clone(&decoded, &clone, &clone_pool);
      empty_amqp_pool(&clone_pool);
    }
  }
  empty_amqp_pool(&pool);
}

// First byte selects the codec; the remainder is the payload, framed like the
// matching decode fuzzer (method-id / class-id prefix where applicable).
extern int LLVMFuzzerTestOneInput(const char *data, size_t size) {
  const uint8_t *bytes = (const uint8_t *)data;

  if (size < 1) {
    return 0;
  }
  switch (bytes[0] % 3) {
    case 0:
      roundtrip_method(bytes + 1, size - 1);
      break;
    case 1:
      roundtrip_properties(bytes + 1, size - 1);
      break;
    default:
      roundtrip_table(bytes + 1, size - 1);
      break;
  }
  return 0;
}
