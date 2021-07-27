// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

struct pipeline {
  int pid;
  int infd;
};

extern void pipeline(const char *const *argv, struct pipeline *pl);
extern int finish_pipeline(struct pipeline *pl);
