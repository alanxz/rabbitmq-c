// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "amqp_private.h"

/* Regression test for GHSA-jgjf-7fwf-f3c7: a size_t integer overflow in
 * amqp_decode_bytes bypassed the bounds check on 32-bit systems, producing an
 * out-of-bounds amqp_bytes_t (information disclosure / crash). The check must
 * reject any (offset, len) pair that would read past the end of the buffer,
 * including ones where offset + len wraps around SIZE_MAX. */

static int failures = 0;

static void expect_reject(const char *name, amqp_bytes_t encoded, size_t offset,
                          size_t len) {
  amqp_bytes_t output;
  size_t off = offset;
  output.bytes = NULL;
  output.len = 0;
  if (amqp_decode_bytes(encoded, &off, &output, len)) {
    fprintf(stderr,
            "FAIL %s: amqp_decode_bytes accepted an out-of-bounds length "
            "(offset=%zu len=%zu buffer=%zu) -> output.len=%zu\n",
            name, offset, len, encoded.len, output.len);
    failures++;
  }
}

static void expect_accept(const char *name, amqp_bytes_t encoded, size_t offset,
                          size_t len) {
  amqp_bytes_t output;
  size_t off = offset;
  output.bytes = NULL;
  output.len = 0;
  if (!amqp_decode_bytes(encoded, &off, &output, len)) {
    fprintf(stderr,
            "FAIL %s: amqp_decode_bytes rejected a valid length "
            "(offset=%zu len=%zu buffer=%zu)\n",
            name, offset, len, encoded.len);
    failures++;
    return;
  }
  if (output.len != len || output.bytes != amqp_offset(encoded.bytes, offset)) {
    fprintf(stderr, "FAIL %s: amqp_decode_bytes produced wrong output\n", name);
    failures++;
  }
}

int main(void) {
  char buffer[16];
  amqp_bytes_t encoded;
  encoded.bytes = buffer;
  encoded.len = sizeof(buffer);

  /* Normal, in-bounds decodes still work. */
  expect_accept("full buffer", encoded, 0, sizeof(buffer));
  expect_accept("partial at offset", encoded, 4, 8);
  expect_accept("zero length", encoded, 8, 0);

  /* Plain out-of-bounds (no overflow) is rejected. */
  expect_reject("len past end", encoded, 0, sizeof(buffer) + 1);
  expect_reject("offset past end", encoded, sizeof(buffer) + 1, 0);

  /* The core of the advisory: a wire length large enough that offset + len
   * wraps around SIZE_MAX. On 32-bit platforms a uint32_t length of
   * 0xFFFFFFF5 with a small offset wraps to a tiny value; on any platform we
   * can force the wrap with a len near SIZE_MAX. Both must be rejected rather
   * than producing a multi-gigabyte amqp_bytes_t into a small buffer. */
  expect_reject("overflow to zero", encoded, 11, (size_t)0 - 11);
  expect_reject("overflow wraps small", encoded, 16, (size_t)0 - 8);
  expect_reject("max len", encoded, 1, (size_t)-1);
  expect_reject("32-bit style len", encoded, 11, (size_t)0xFFFFFFF5u);

  if (failures) {
    fprintf(stderr, "%d test(s) failed\n", failures);
    return 1;
  }
  printf("all amqp_decode_bytes bounds tests passed\n");
  return 0;
}
