/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * The Original Code is librabbitmq.
 *
 * The Initial Developers of the Original Code are LShift Ltd, Cohesive
 * Financial Technologies LLC, and Rabbit Technologies Ltd.  Portions
 * created before 22-Nov-2008 00:00:00 GMT by LShift Ltd, Cohesive
 * Financial Technologies LLC, or Rabbit Technologies Ltd are Copyright
 * (C) 2007-2008 LShift Ltd, Cohesive Financial Technologies LLC, and
 * Rabbit Technologies Ltd.
 *
 * Portions created by LShift Ltd are Copyright (C) 2007-2009 LShift
 * Ltd. Portions created by Cohesive Financial Technologies LLC are
 * Copyright (C) 2007-2009 Cohesive Financial Technologies
 * LLC. Portions created by Rabbit Technologies Ltd are Copyright (C)
 * 2007-2009 Rabbit Technologies Ltd.
 *
 * Portions created by Tony Garnock-Jones are Copyright (C) 2009-2010
 * LShift Ltd and Tony Garnock-Jones.
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2 or later (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GPL, and not to allow others to use your
 * version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the
 * notice and other provisions required by the GPL. If you do not
 * delete the provisions above, a recipient may use your version of
 * this file under the terms of any one of the MPL or the GPL.
 *
 * ***** END LICENSE BLOCK *****
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <inttypes.h>

#include <amqp.h>
#include <amqp_framing.h>
#include <amqp_private.h>

#include <unistd.h>
#include <assert.h>

#include <math.h>

static void dump_indent(int indent) {
  int i;
  for (i = 0; i < indent; i++) { putchar(' '); }
}

static void dump_value(int indent, amqp_field_value_t v) {
  dump_indent(indent);
  putchar(v.kind);
  putchar(' ');
  switch (v.kind) {
    case AMQP_FIELD_KIND_BOOLEAN: puts(v.value.boolean ? "true" : "false"); break;
    case AMQP_FIELD_KIND_I8: printf("%"PRId8"\n", v.value.i8); break;
    case AMQP_FIELD_KIND_U8: printf("%"PRIu8"\n", v.value.u8); break;
    case AMQP_FIELD_KIND_I16: printf("%"PRId16"\n", v.value.i16); break;
    case AMQP_FIELD_KIND_U16: printf("%"PRIu16"\n", v.value.u16); break;
    case AMQP_FIELD_KIND_I32: printf("%"PRId32"\n", v.value.i32); break;
    case AMQP_FIELD_KIND_U32: printf("%"PRIu32"\n", v.value.u32); break;
    case AMQP_FIELD_KIND_I64: printf("%"PRId64"\n", v.value.i64); break;
    case AMQP_FIELD_KIND_F32: printf("%g\n", (double) v.value.f32); break;
    case AMQP_FIELD_KIND_F64: printf("%g\n", v.value.f64); break;
    case AMQP_FIELD_KIND_DECIMAL:
      printf("%d:::%u\n", v.value.decimal.decimals, v.value.decimal.value); break;
    case AMQP_FIELD_KIND_UTF8:
      printf("%.*s\n", (int) v.value.bytes.len, (char *) v.value.bytes.bytes); break;
    case AMQP_FIELD_KIND_BYTES:
      {
	int i;
	for (i = 0; i < v.value.bytes.len; i++) {
	  printf("%02x", ((char *) v.value.bytes.bytes)[i]);
	}
	putchar('\n');
      }
      break;
    case AMQP_FIELD_KIND_ARRAY:
      putchar('\n');
      {
	int i;
	for (i = 0; i < v.value.array.num_entries; i++) {
	  dump_value(indent + 2, v.value.array.entries[i]);
	}
      }
      break;
    case AMQP_FIELD_KIND_TIMESTAMP: printf("%"PRIu64"\n", v.value.u64); break;
    case AMQP_FIELD_KIND_TABLE:
      putchar('\n');
      {
	int i;
	for (i = 0; i < v.value.table.num_entries; i++) {
	  dump_indent(indent + 2);
	  printf("%.*s ->\n",
		 (int) v.value.table.entries[i].key.len,
		 (char *) v.value.table.entries[i].key.bytes);
	  dump_value(indent + 4, v.value.table.entries[i].value);
	}
      }
      break;
    case AMQP_FIELD_KIND_VOID: putchar('\n'); break;
    default:
      printf("???\n");
      break;
  }
}

static void test_table_codec(void) {
  amqp_table_entry_t inner_entries[2] =
    { AMQP_TABLE_ENTRY_I32("one", 54321),
      AMQP_TABLE_ENTRY_UTF8("two", amqp_cstring_bytes("A long string")) };
  amqp_table_t inner_table = { .num_entries = sizeof(inner_entries) / sizeof(inner_entries[0]),
			       .entries = &inner_entries[0] };

  amqp_field_value_t inner_values[2] =
    { AMQP_FIELD_VALUE_I32(54321),
      AMQP_FIELD_VALUE_UTF8(amqp_cstring_bytes("A long string")) };
  amqp_array_t inner_array = { .num_entries = sizeof(inner_values) / sizeof(inner_values[0]),
			       .entries = &inner_values[0] };

  amqp_table_entry_t entries[14] =
    { AMQP_TABLE_ENTRY_UTF8("longstr", amqp_cstring_bytes("Here is a long string")),
      AMQP_TABLE_ENTRY_I32("signedint", 12345),
      AMQP_TABLE_ENTRY_DECIMAL("decimal", AMQP_DECIMAL(3, 123456)),
      AMQP_TABLE_ENTRY_TIMESTAMP("timestamp", 109876543209876),
      AMQP_TABLE_ENTRY_TABLE("table", inner_table),
      AMQP_TABLE_ENTRY_I8("byte", 255),
      AMQP_TABLE_ENTRY_I64("long", 1234567890),
      AMQP_TABLE_ENTRY_I16("short", 655),
      AMQP_TABLE_ENTRY_BOOLEAN("bool", 1),
      AMQP_TABLE_ENTRY_BYTES("binary", amqp_cstring_bytes("a binary string")),
      AMQP_TABLE_ENTRY_VOID("void"),
      AMQP_TABLE_ENTRY_ARRAY("array", inner_array),
      AMQP_TABLE_ENTRY_F32("float", M_PI),
      AMQP_TABLE_ENTRY_F64("double", M_PI) };
  amqp_table_t table = { .num_entries = sizeof(entries) / sizeof(entries[0]),
			 .entries = &entries[0] };

  uint8_t pre_encoded_table[] = {
    0x00, 0x00, 0x00, 0xff, 0x07, 0x6c, 0x6f, 0x6e,
    0x67, 0x73, 0x74, 0x72, 0x53, 0x00, 0x00, 0x00,
    0x15, 0x48, 0x65, 0x72, 0x65, 0x20, 0x69, 0x73,
    0x20, 0x61, 0x20, 0x6c, 0x6f, 0x6e, 0x67, 0x20,
    0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x09, 0x73,
    0x69, 0x67, 0x6e, 0x65, 0x64, 0x69, 0x6e, 0x74,
    0x49, 0x00, 0x00, 0x30, 0x39, 0x07, 0x64, 0x65,
    0x63, 0x69, 0x6d, 0x61, 0x6c, 0x44, 0x03, 0x00,
    0x01, 0xe2, 0x40, 0x09, 0x74, 0x69, 0x6d, 0x65,
    0x73, 0x74, 0x61, 0x6d, 0x70, 0x54, 0x00, 0x00,
    0x63, 0xee, 0xa0, 0x53, 0xc1, 0x94, 0x05, 0x74,
    0x61, 0x62, 0x6c, 0x65, 0x46, 0x00, 0x00, 0x00,
    0x1f, 0x03, 0x6f, 0x6e, 0x65, 0x49, 0x00, 0x00,
    0xd4, 0x31, 0x03, 0x74, 0x77, 0x6f, 0x53, 0x00,
    0x00, 0x00, 0x0d, 0x41, 0x20, 0x6c, 0x6f, 0x6e,
    0x67, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67,
    0x04, 0x62, 0x79, 0x74, 0x65, 0x62, 0xff, 0x04,
    0x6c, 0x6f, 0x6e, 0x67, 0x6c, 0x00, 0x00, 0x00,
    0x00, 0x49, 0x96, 0x02, 0xd2, 0x05, 0x73, 0x68,
    0x6f, 0x72, 0x74, 0x73, 0x02, 0x8f, 0x04, 0x62,
    0x6f, 0x6f, 0x6c, 0x74, 0x01, 0x06, 0x62, 0x69,
    0x6e, 0x61, 0x72, 0x79, 0x78, 0x00, 0x00, 0x00,
    0x0f, 0x61, 0x20, 0x62, 0x69, 0x6e, 0x61, 0x72,
    0x79, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67,
    0x04, 0x76, 0x6f, 0x69, 0x64, 0x56, 0x05, 0x61,
    0x72, 0x72, 0x61, 0x79, 0x41, 0x00, 0x00, 0x00,
    0x17, 0x49, 0x00, 0x00, 0xd4, 0x31, 0x53, 0x00,
    0x00, 0x00, 0x0d, 0x41, 0x20, 0x6c, 0x6f, 0x6e,
    0x67, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67,
    0x05, 0x66, 0x6c, 0x6f, 0x61, 0x74, 0x66, 0x40,
    0x49, 0x0f, 0xdb, 0x06, 0x64, 0x6f, 0x75, 0x62,
    0x6c, 0x65, 0x64, 0x40, 0x09, 0x21, 0xfb, 0x54,
    0x44, 0x2d, 0x18
  };

  amqp_pool_t pool;
  int result;

  printf("AAAAAAAAAA\n");
  dump_value(0, (amqp_field_value_t) AMQP_FIELD_VALUE_TABLE(table));

  init_amqp_pool(&pool, 4096);

  {
    amqp_bytes_t decoding_bytes = { .len = sizeof(pre_encoded_table),
				    .bytes = pre_encoded_table };
    amqp_table_t decoded;
    int decoding_offset = 0;
    result = amqp_decode_table(decoding_bytes, &pool, &decoded, &decoding_offset);
    if (result < 0) {
      printf("Table decoding failed: %d (%s)\n", result,
	     amqp_error_string(-result));
      abort();
    }
    printf("BBBBBBBBBB\n");
    dump_value(0, (amqp_field_value_t) AMQP_FIELD_VALUE_TABLE(decoded));
  }

  {
    uint8_t encoding_buffer[4096];
    amqp_bytes_t encoding_result;
    int offset = 0;

    memset(&encoding_buffer[0], 0, sizeof(encoding_buffer));
    encoding_result.len = sizeof(encoding_buffer);
    encoding_result.bytes = &encoding_buffer[0];

    result = amqp_encode_table(encoding_result, &table, &offset);
    if (result < 0) {
      printf("Table encoding failed: %d (%s)\n", result, 
	     amqp_error_string(-result));
      abort();
    }

    if (offset != sizeof(pre_encoded_table)) {
      printf("Offset should be %d, was %d\n", (int) sizeof(pre_encoded_table), offset);
      abort();
    }

    result = memcmp(pre_encoded_table, encoding_buffer, offset);
    if (result != 0) {
      printf("Table encoding differed, result = %d\n", result);
      abort();
    }
  }

  empty_amqp_pool(&pool);
}

int main(int argc, char const * const *argv) {
  amqp_table_entry_t entries[8] =
    { AMQP_TABLE_ENTRY_UTF8("zebra", amqp_cstring_bytes("last")),
      AMQP_TABLE_ENTRY_UTF8("aardvark", amqp_cstring_bytes("first")),
      AMQP_TABLE_ENTRY_UTF8("middle", amqp_cstring_bytes("third")),
      AMQP_TABLE_ENTRY_I32("number", 1234),
      AMQP_TABLE_ENTRY_DECIMAL("decimal", AMQP_DECIMAL(2, 1234)),
      AMQP_TABLE_ENTRY_TIMESTAMP("time", (uint64_t) 1234123412341234LL),
      AMQP_TABLE_ENTRY_UTF8("beta", amqp_cstring_bytes("second")),
      AMQP_TABLE_ENTRY_UTF8("wombat", amqp_cstring_bytes("fourth")) };
  amqp_table_t table = { .num_entries = sizeof(entries) / sizeof(entries[0]),
			 .entries = &entries[0] };

  union {
    uint32_t i;
    float f;
  } vi;
  union {
    uint64_t l;
    double d;
  } vl;

  vi.f = M_PI;
  if ((sizeof(float) != 4) || (vi.i != 0x40490fdb)) {
    printf("*** ERROR: single floating point encoding does not work as expected\n");
    printf("sizeof float is %lu, float is %g, u32 is 0x%08lx\n",
	   (unsigned long)sizeof(float),
	   vi.f,
	   (unsigned long) vi.i);
  }

  vl.d = M_PI;
  if ((sizeof(double) != 8) || (vl.l != 0x400921fb54442d18L)) {
    printf("*** ERROR: double floating point encoding does not work as expected\n");
    printf("sizeof double is %lu, double is %g, u64 is 0x%16"PRIx64"\n",
	   (unsigned long)sizeof(double),
	   vl.d, vl.l);
  }

  test_table_codec();

  qsort(table.entries, table.num_entries, sizeof(amqp_table_entry_t), &amqp_table_entry_cmp);

  printf("----------\n");
  dump_value(0, (amqp_field_value_t) AMQP_FIELD_VALUE_TABLE(table));

  return 0;
}
