// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <windef.h>

struct pipeline {
  HANDLE proc_handle;
  int infd;
};

extern void pipeline(const char *const *argv, struct pipeline *pl);
extern int finish_pipeline(struct pipeline *pl);
