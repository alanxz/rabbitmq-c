// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <stdint.h>

#include <windows.h>

uint64_t now_microseconds(void) {
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  return (((uint64_t)ft.dwHighDateTime << 32) | (uint64_t)ft.dwLowDateTime) /
         10;
}

void microsleep(int usec) { Sleep(usec / 1000); }
