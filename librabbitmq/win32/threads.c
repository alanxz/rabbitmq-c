// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include "threads.h"

#include <stdlib.h>

DWORD pthread_self(void) { return GetCurrentThreadId(); }

int pthread_mutex_init(pthread_mutex_t *mutex, void *attr) {
  if (!mutex) {
    return 1;
  }
  InitializeSRWLock(mutex);
  return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
  if (!mutex) {
    return 1;
  }
  AcquireSRWLockExclusive(mutex);
  return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
  if (!mutex) {
    return 1;
  }
  ReleaseSRWLockExclusive(mutex);
  return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
  /* SRW's do not require destruction. */
  return 0;
}
