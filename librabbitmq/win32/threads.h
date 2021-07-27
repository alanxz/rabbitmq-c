// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#ifndef AMQP_THREAD_H
#define AMQP_THREAD_H

#if !defined(WINVER) || defined(__MINGW32__) || defined(__MINGW64__)
#ifdef WINVER
#undef WINVER
#endif
/* Windows Vista or newer */
#define WINVER 0x0600
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef SRWLOCK pthread_mutex_t;
#define PTHREAD_MUTEX_INITIALIZER SRWLOCK_INIT;

DWORD pthread_self(void);

int pthread_mutex_init(pthread_mutex_t *, void *attr);
int pthread_mutex_lock(pthread_mutex_t *);
int pthread_mutex_unlock(pthread_mutex_t *);
int pthread_mutex_destroy(pthread_mutex_t *);

#endif /* AMQP_THREAD_H */
