/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
 *
 * Portions created by VMware are Copyright (c) 2007-2012 VMware, Inc.
 * All Rights Reserved.
 *
 * Portions created by Tony Garnock-Jones are Copyright (c) 2009-2010
 * VMware, Inc. and Tony Garnock-Jones. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ***** END LICENSE BLOCK *****
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "amqp_private.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_ARRAY_SIZE 16
#define INITIAL_TABLE_SIZE 16

static int amqp_decode_field_value(amqp_bytes_t encoded,
				   amqp_pool_t *pool,
				   amqp_field_value_t *entry,
				   size_t *offset);

static int amqp_encode_field_value(amqp_bytes_t encoded,
				   amqp_field_value_t *entry,
				   size_t *offset);

/*---------------------------------------------------------------------------*/

static int amqp_decode_array(amqp_bytes_t encoded,
			     amqp_pool_t *pool,
			     amqp_array_t *output,
			     size_t *offset)
{
  uint32_t arraysize;
  int num_entries = 0;
  int allocated_entries = INITIAL_ARRAY_SIZE;
  amqp_field_value_t *entries;
  size_t limit;
  int res;

  if (!amqp_decode_32(encoded, offset, &arraysize))
    return -ERROR_BAD_AMQP_DATA;

  entries = malloc(allocated_entries * sizeof(amqp_field_value_t));
  if (entries == NULL)
    return -ERROR_NO_MEMORY;

  limit = *offset + arraysize;
  while (*offset < limit) {
    if (num_entries >= allocated_entries) {
      void *newentries;
      allocated_entries = allocated_entries * 2;
      newentries = realloc(entries, allocated_entries * sizeof(amqp_field_value_t));
      res = -ERROR_NO_MEMORY;
      if (newentries == NULL)
	goto out;

      entries = newentries;
    }

    res = amqp_decode_field_value(encoded, pool, &entries[num_entries],
				  offset);
    if (res < 0)
      goto out;

    num_entries++;
  }

  output->num_entries = num_entries;
  output->entries = amqp_pool_alloc(pool, num_entries * sizeof(amqp_field_value_t));
  res = -ERROR_NO_MEMORY;
  /* NULL is legitimate if we requested a zero-length block. */
  if (output->entries == NULL && num_entries > 0)
    goto out;

  memcpy(output->entries, entries, num_entries * sizeof(amqp_field_value_t));
  res = 0;

 out:
  free(entries);
  return res;
}

int amqp_decode_table(amqp_bytes_t encoded,
		      amqp_pool_t *pool,
		      amqp_table_t *output,
		      size_t *offset)
{
  uint32_t tablesize;
  int num_entries = 0;
  amqp_table_entry_t *entries;
  int allocated_entries = INITIAL_TABLE_SIZE;
  size_t limit;
  int res;

  if (!amqp_decode_32(encoded, offset, &tablesize))
    return -ERROR_BAD_AMQP_DATA;

  entries = malloc(allocated_entries * sizeof(amqp_table_entry_t));
  if (entries == NULL)
    return -ERROR_NO_MEMORY;

  limit = *offset + tablesize;
  while (*offset < limit) {
    uint8_t keylen;

    res = -ERROR_BAD_AMQP_DATA;
    if (!amqp_decode_8(encoded, offset, &keylen))
      goto out;

    if (num_entries >= allocated_entries) {
      void *newentries;
      allocated_entries = allocated_entries * 2;
      newentries = realloc(entries, allocated_entries * sizeof(amqp_table_entry_t));
      res = -ERROR_NO_MEMORY;
      if (newentries == NULL)
	goto out;

      entries = newentries;
    }

    res = -ERROR_BAD_AMQP_DATA;
    if (!amqp_decode_bytes(encoded, offset, &entries[num_entries].key, keylen))
      goto out;

    res = amqp_decode_field_value(encoded, pool, &entries[num_entries].value,
				  offset);
    if (res < 0)
      goto out;

    num_entries++;
  }

  output->num_entries = num_entries;
  output->entries = amqp_pool_alloc(pool, num_entries * sizeof(amqp_table_entry_t));
  res = -ERROR_NO_MEMORY;
  /* NULL is legitimate if we requested a zero-length block. */
  if (output->entries == NULL && num_entries > 0)
    goto out;

  memcpy(output->entries, entries, num_entries * sizeof(amqp_table_entry_t));
  res = 0;

 out:
  free(entries);
  return res;
}

static int amqp_decode_field_value(amqp_bytes_t encoded,
				   amqp_pool_t *pool,
				   amqp_field_value_t *entry,
				   size_t *offset)
{
  int res = -ERROR_BAD_AMQP_DATA;

  if (!amqp_decode_8(encoded, offset, &entry->kind))
    goto out;

#define TRIVIAL_FIELD_DECODER(bits) if (!amqp_decode_##bits(encoded, offset, &entry->value.u##bits)) goto out; break
#define SIMPLE_FIELD_DECODER(bits, dest, how) { uint##bits##_t val; if (!amqp_decode_##bits(encoded, offset, &val)) goto out; entry->value.dest = how; } break

  switch (entry->kind) {
  case AMQP_FIELD_KIND_BOOLEAN:
    SIMPLE_FIELD_DECODER(8, boolean, val ? 1 : 0);

  case AMQP_FIELD_KIND_I8:
    SIMPLE_FIELD_DECODER(8, i8, (int8_t)val);
  case AMQP_FIELD_KIND_U8:
    TRIVIAL_FIELD_DECODER(8);

  case AMQP_FIELD_KIND_I16:
    SIMPLE_FIELD_DECODER(16, i16, (int16_t)val);
  case AMQP_FIELD_KIND_U16:
    TRIVIAL_FIELD_DECODER(16);

  case AMQP_FIELD_KIND_I32:
    SIMPLE_FIELD_DECODER(32, i32, (int32_t)val);
  case AMQP_FIELD_KIND_U32:
    TRIVIAL_FIELD_DECODER(32);

  case AMQP_FIELD_KIND_I64:
    SIMPLE_FIELD_DECODER(64, i64, (int64_t)val);
  case AMQP_FIELD_KIND_U64:
    TRIVIAL_FIELD_DECODER(64);

  case AMQP_FIELD_KIND_F32:
    TRIVIAL_FIELD_DECODER(32);
    /* and by punning, f32 magically gets the right value...! */

  case AMQP_FIELD_KIND_F64:
    TRIVIAL_FIELD_DECODER(64);
    /* and by punning, f64 magically gets the right value...! */

  case AMQP_FIELD_KIND_DECIMAL:
    if (!amqp_decode_8(encoded, offset, &entry->value.decimal.decimals)
	|| !amqp_decode_32(encoded, offset, &entry->value.decimal.value))
      goto out;
    break;

  case AMQP_FIELD_KIND_UTF8:
    /* AMQP_FIELD_KIND_UTF8 and AMQP_FIELD_KIND_BYTES have the
       same implementation, but different interpretations. */
    /* fall through */
  case AMQP_FIELD_KIND_BYTES: {
    uint32_t len;
    if (!amqp_decode_32(encoded, offset, &len)
	|| !amqp_decode_bytes(encoded, offset, &entry->value.bytes, len))
      goto out;
    break;
  }

  case AMQP_FIELD_KIND_ARRAY:
    res = amqp_decode_array(encoded, pool, &(entry->value.array), offset);
    goto out;

  case AMQP_FIELD_KIND_TIMESTAMP:
    TRIVIAL_FIELD_DECODER(64);

  case AMQP_FIELD_KIND_TABLE:
    res = amqp_decode_table(encoded, pool, &(entry->value.table), offset);
    goto out;

  case AMQP_FIELD_KIND_VOID:
    break;

  default:
    goto out;
  }

  res = 0;

 out:
  return res;
}

/*---------------------------------------------------------------------------*/

static int amqp_encode_array(amqp_bytes_t encoded,
			     amqp_array_t *input,
			     size_t *offset)
{
  size_t start = *offset;
  int i, res;

  *offset += 4; /* size of the array gets filled in later on */

  for (i = 0; i < input->num_entries; i++) {
    res = amqp_encode_field_value(encoded, &input->entries[i], offset);
    if (res < 0)
      goto out;
  }

  if (amqp_encode_32(encoded, &start, *offset - start - 4))
    res = 0;
  else
    res = -ERROR_BAD_AMQP_DATA;

 out:
  return res;
}

int amqp_encode_table(amqp_bytes_t encoded,
		      amqp_table_t *input,
		      size_t *offset)
{
  size_t start = *offset;
  int i, res;

  *offset += 4; /* size of the table gets filled in later on */

  for (i = 0; i < input->num_entries; i++) {
    res = amqp_encode_8(encoded, offset, input->entries[i].key.len);
    if (res < 0)
      goto out;

    res = amqp_encode_bytes(encoded, offset, input->entries[i].key);
    if (res < 0)
      goto out;

    res = amqp_encode_field_value(encoded, &input->entries[i].value, offset);
    if (res < 0)
      goto out;
  }

  if (amqp_encode_32(encoded, &start, *offset - start - 4))
    res = 0;
  else
    res = -ERROR_BAD_AMQP_DATA;

 out:
  return res;
}

static int amqp_encode_field_value(amqp_bytes_t encoded,
				   amqp_field_value_t *entry,
				   size_t *offset)
{
  int res = -ERROR_BAD_AMQP_DATA;

  if (!amqp_encode_8(encoded, offset, entry->kind))
    goto out;

#define FIELD_ENCODER(bits, val) if (!amqp_encode_##bits(encoded, offset, val)) goto out; break

  switch (entry->kind) {
  case AMQP_FIELD_KIND_BOOLEAN:
    FIELD_ENCODER(8, entry->value.boolean ? 1 : 0);

  case AMQP_FIELD_KIND_I8:
    FIELD_ENCODER(8, entry->value.i8);
  case AMQP_FIELD_KIND_U8:
    FIELD_ENCODER(8, entry->value.u8);

  case AMQP_FIELD_KIND_I16:
    FIELD_ENCODER(16, entry->value.i16);
  case AMQP_FIELD_KIND_U16:
    FIELD_ENCODER(16, entry->value.u16);

  case AMQP_FIELD_KIND_I32:
    FIELD_ENCODER(32, entry->value.i32);
  case AMQP_FIELD_KIND_U32:
    FIELD_ENCODER(32, entry->value.u32);

  case AMQP_FIELD_KIND_I64:
    FIELD_ENCODER(64, entry->value.i64);
  case AMQP_FIELD_KIND_U64:
    FIELD_ENCODER(64, entry->value.u64);

  case AMQP_FIELD_KIND_F32:
    /* by punning, u32 magically gets the right value...! */
    FIELD_ENCODER(32, entry->value.u32);

  case AMQP_FIELD_KIND_F64:
    /* by punning, u64 magically gets the right value...! */
    FIELD_ENCODER(64, entry->value.u64);

  case AMQP_FIELD_KIND_DECIMAL:
    if (!amqp_encode_8(encoded, offset, entry->value.decimal.decimals)
	|| !amqp_encode_32(encoded, offset, entry->value.decimal.value))
      goto out;
    break;

  case AMQP_FIELD_KIND_UTF8:
    /* AMQP_FIELD_KIND_UTF8 and AMQP_FIELD_KIND_BYTES have the
       same implementation, but different interpretations. */
    /* fall through */
  case AMQP_FIELD_KIND_BYTES:
    if (!amqp_encode_32(encoded, offset, entry->value.bytes.len)
	|| !amqp_encode_bytes(encoded, offset, entry->value.bytes))
      goto out;
    break;

  case AMQP_FIELD_KIND_ARRAY:
    res = amqp_encode_array(encoded, &entry->value.array, offset);
    goto out;

  case AMQP_FIELD_KIND_TIMESTAMP:
    FIELD_ENCODER(64, entry->value.u64);

  case AMQP_FIELD_KIND_TABLE:
    res = amqp_encode_table(encoded, &entry->value.table, offset);
    goto out;

  case AMQP_FIELD_KIND_VOID:
    break;

  default:
    abort();
  }

  res = 0;

 out:
  return res;
}

/*---------------------------------------------------------------------------*/

int amqp_table_entry_cmp(void const *entry1, void const *entry2) {
  amqp_table_entry_t const *p1 = (amqp_table_entry_t const *) entry1;
  amqp_table_entry_t const *p2 = (amqp_table_entry_t const *) entry2;

  int d;
  size_t minlen;

  minlen = p1->key.len;
  if (p2->key.len < minlen) minlen = p2->key.len;

  d = memcmp(p1->key.bytes, p2->key.bytes, minlen);
  if (d != 0) {
    return d;
  }

  return p1->key.len - p2->key.len;
}
