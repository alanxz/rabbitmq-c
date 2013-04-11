/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
#ifndef AMQP_THREAD_H
#define AMQP_THREAD_H

#include <Windows.h>

typedef CRITICAL_SECTION *pthread_mutex_t;
typedef int pthread_once_t;

DWORD pthread_self(void);

int pthread_mutex_init(pthread_mutex_t *, void *attr);
int pthread_mutex_lock(pthread_mutex_t *);
int pthread_mutex_unlock(pthread_mutex_t *);
#endif /* AMQP_THREAD_H */
