// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "compat.h"

int asprintf(char **strp, const char *fmt, ...) {
  va_list ap;
  int len;

  va_start(ap, fmt);
  len = _vscprintf(fmt, ap);
  va_end(ap);

  *strp = malloc(len + 1);
  if (!*strp) {
    return -1;
  }

  va_start(ap, fmt);
  _vsnprintf(*strp, len + 1, fmt, ap);
  va_end(ap);

  (*strp)[len] = 0;
  return len;
}
