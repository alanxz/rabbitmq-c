/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * The Original Code is librabbitmq.
 *
 * The Initial Developer of the Original Code is VMware, Inc.
 * Portions created by VMware are Copyright (c) 2007-2011 VMware, Inc.
 *
 * Portions created by Tony Garnock-Jones are Copyright (c) 2009-2010
 * VMware, Inc. and Tony Garnock-Jones.
 *
 * All rights reserved.
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

#include <inttypes.h>

#include <amqp.h>

#include <math.h>

#define M_PI 3.14159265358979323846264338327

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

static uint8_t pre_encoded_table[] = {
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

static void test_table_codec(void) {
  amqp_pool_t pool;
  int result;

  amqp_table_entry_t inner_entries[2];
  amqp_table_t inner_table;

  amqp_field_value_t inner_values[2];
  amqp_array_t inner_array;

  amqp_table_entry_t entries[14];
  amqp_table_t table;

  inner_entries[0].key = amqp_cstring_bytes("one");
  inner_entries[0].value.kind = AMQP_FIELD_KIND_I32;
  inner_entries[0].value.value.i32 = 54321;

  inner_entries[1].key = amqp_cstring_bytes("two");
  inner_entries[1].value.kind = AMQP_FIELD_KIND_UTF8;
  inner_entries[1].value.value.bytes = amqp_cstring_bytes("A long string");

  inner_table.num_entries = 2;
  inner_table.entries = inner_entries;

  inner_values[0].kind = AMQP_FIELD_KIND_I32;
  inner_values[0].value.i32 = 54321;

  inner_values[1].kind = AMQP_FIELD_KIND_UTF8;
  inner_values[1].value.bytes = amqp_cstring_bytes("A long string");

  inner_array.num_entries = 2;
  inner_array.entries = inner_values;

  entries[0].key = amqp_cstring_bytes("longstr");
  entries[0].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[0].value.value.bytes = amqp_cstring_bytes("Here is a long string");

  entries[1].key = amqp_cstring_bytes("signedint");
  entries[1].value.kind = AMQP_FIELD_KIND_I32;
  entries[1].value.value.i32 = 12345;

  entries[2].key = amqp_cstring_bytes("decimal");
  entries[2].value.kind = AMQP_FIELD_KIND_DECIMAL;
  entries[2].value.value.decimal.decimals = 3;
  entries[2].value.value.decimal.value = 123456;

  entries[3].key = amqp_cstring_bytes("timestamp");
  entries[3].value.kind = AMQP_FIELD_KIND_TIMESTAMP;
  entries[3].value.value.u64 = 109876543209876;

  entries[4].key = amqp_cstring_bytes("table");
  entries[4].value.kind = AMQP_FIELD_KIND_TABLE;
  entries[4].value.value.table = inner_table;

  entries[5].key = amqp_cstring_bytes("byte");
  entries[5].value.kind = AMQP_FIELD_KIND_I8;
  entries[5].value.value.i8 = (int8_t)255;

  entries[6].key = amqp_cstring_bytes("long");
  entries[6].value.kind = AMQP_FIELD_KIND_I64;
  entries[6].value.value.i64 = 1234567890;

  entries[7].key = amqp_cstring_bytes("short");
  entries[7].value.kind = AMQP_FIELD_KIND_I16;
  entries[7].value.value.i16 = 655;

  entries[8].key = amqp_cstring_bytes("bool");
  entries[8].value.kind = AMQP_FIELD_KIND_BOOLEAN;
  entries[8].value.value.boolean = 1;

  entries[9].key = amqp_cstring_bytes("binary");
  entries[9].value.kind = AMQP_FIELD_KIND_BYTES;
  entries[9].value.value.bytes = amqp_cstring_bytes("a binary string");

  entries[10].key = amqp_cstring_bytes("void");
  entries[10].value.kind = AMQP_FIELD_KIND_VOID;

  entries[11].key = amqp_cstring_bytes("array");
  entries[11].value.kind = AMQP_FIELD_KIND_ARRAY;
  entries[11].value.value.array = inner_array;

  entries[12].key = amqp_cstring_bytes("float");
  entries[12].value.kind = AMQP_FIELD_KIND_F32;
  entries[12].value.value.f32 = M_PI;

  entries[13].key = amqp_cstring_bytes("double");
  entries[13].value.kind = AMQP_FIELD_KIND_F64;
  entries[13].value.value.f64 = M_PI;

  table.num_entries = 14;
  table.entries = entries;

  printf("AAAAAAAAAA\n");

  {
    amqp_field_value_t val;
    val.kind = AMQP_FIELD_KIND_TABLE;
    val.value.table = table;
    dump_value(0, val);
  }

  init_amqp_pool(&pool, 4096);

  {
    amqp_table_t decoded;
    size_t decoding_offset = 0;
    amqp_bytes_t decoding_bytes;
    decoding_bytes.len = sizeof(pre_encoded_table);
    decoding_bytes.bytes = pre_encoded_table;

    result = amqp_decode_table(decoding_bytes, &pool, &decoded, &decoding_offset);
    if (result < 0) {
      char *errstr = amqp_error_string(-result);
      printf("Table decoding failed: %d (%s)\n", result, errstr);
      free(errstr);
      abort();
    }
    printf("BBBBBBBBBB\n");

    {
      amqp_field_value_t val;
      val.kind = AMQP_FIELD_KIND_TABLE;
      val.value.table = decoded;

      dump_value(0, val);
    }
  }

  {
    uint8_t encoding_buffer[4096];
    amqp_bytes_t encoding_result;
    size_t offset = 0;

    memset(&encoding_buffer[0], 0, sizeof(encoding_buffer));
    encoding_result.len = sizeof(encoding_buffer);
    encoding_result.bytes = &encoding_buffer[0];

    result = amqp_encode_table(encoding_result, &table, &offset);
    if (result < 0) {
      char *errstr = amqp_error_string(-result);
      printf("Table encoding failed: %d (%s)\n", result, errstr);
      free(errstr);
      abort();
    }

    if (offset != sizeof(pre_encoded_table)) {
      printf("Offset should be %d, was %d\n", (int) sizeof(pre_encoded_table), (int)offset);
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
  amqp_table_entry_t entries[8];
  amqp_table_t table;

  union {
    uint32_t i;
    float f;
  } vi;
  union {
    uint64_t l;
    double d;
  } vl;

  entries[0].key = amqp_cstring_bytes("zebra");
  entries[0].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[0].value.value.bytes = amqp_cstring_bytes("last");

  entries[1].key = amqp_cstring_bytes("aardvark");
  entries[1].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[1].value.value.bytes = amqp_cstring_bytes("first");

  entries[2].key = amqp_cstring_bytes("middle");
  entries[2].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[2].value.value.bytes = amqp_cstring_bytes("third");

  entries[3].key = amqp_cstring_bytes("number");
  entries[3].value.kind = AMQP_FIELD_KIND_I32;
  entries[3].value.value.i32 = 1234;

  entries[4].key = amqp_cstring_bytes("decimal");
  entries[4].value.kind = AMQP_FIELD_KIND_DECIMAL;
  entries[4].value.value.decimal.decimals = 2;
  entries[4].value.value.decimal.value = 1234;

  entries[5].key = amqp_cstring_bytes("time");
  entries[5].value.kind = AMQP_FIELD_KIND_TIMESTAMP;
  entries[5].value.value.u64 = 1234123412341234;

  entries[6].key = amqp_cstring_bytes("beta");
  entries[6].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[6].value.value.bytes = amqp_cstring_bytes("second");

  entries[7].key = amqp_cstring_bytes("wombat");
  entries[7].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[7].value.value.bytes = amqp_cstring_bytes("fourth");

  table.num_entries = 8;
  table.entries = entries;

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

  {
    amqp_field_value_t val;
    val.kind = AMQP_FIELD_KIND_TABLE;
    val.value.table = table;

    dump_value(0, val);
  }

  return 0;
}
