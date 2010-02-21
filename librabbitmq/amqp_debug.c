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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>

static void dump_row(long count, int numinrow, int *chs) {
  int i;

  printf("%08lX:", count - numinrow);

  if (numinrow > 0) {
    for (i = 0; i < numinrow; i++) {
      if (i == 8)
	printf(" :");
      printf(" %02X", chs[i]);
    }
    for (i = numinrow; i < 16; i++) {
      if (i == 8)
	printf(" :");
      printf("   ");
    }
    printf("  ");
    for (i = 0; i < numinrow; i++) {
      if (isprint(chs[i]))
	printf("%c", chs[i]);
      else
	printf(".");
    }
  }
  printf("\n");
}

static int rows_eq(int *a, int *b) {
  int i;

  for (i=0; i<16; i++)
    if (a[i] != b[i])
      return 0;

  return 1;
}

void amqp_dump(void const *buffer, size_t len) {
  unsigned char *buf = (unsigned char *) buffer;
  long count = 0;
  int numinrow = 0;
  int chs[16];
  int oldchs[16];
  int showed_dots = 0;
  int i;

  for (i = 0; i < len; i++) {
    int ch = buf[i];

    if (numinrow == 16) {
      int i;

      if (rows_eq(oldchs, chs)) {
	if (!showed_dots) {
	  showed_dots = 1;
	  printf("          .. .. .. .. .. .. .. .. : .. .. .. .. .. .. .. ..\n");
	}
      } else {
	showed_dots = 0;
	dump_row(count, numinrow, chs);
      }

      for (i=0; i<16; i++)
	oldchs[i] = chs[i];

      numinrow = 0;
    }

    count++;
    chs[numinrow++] = ch;
  }

  dump_row(count, numinrow, chs);

  if (numinrow != 0)
    printf("%08lX:\n", count);
}
