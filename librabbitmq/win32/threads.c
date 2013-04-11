/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
#include "threads.h"

DWORD
pthread_self(void)
{
  return GetCurrentThreadId();
}

int
pthread_mutex_init(pthread_mutex_t *mutex, void *attr)
{
  *mutex = malloc(sizeof(CRITICAL_SECTION));
  if (!*mutex) {
    return 1;
  }
  InitializeCriticalSection(*mutex);
  return 0;
}

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
  if (!*mutex) {
    return 1;
  }

  EnterCriticalSection(*mutex);
  return 0;
}

int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  if (!*mutex) {
    return 1;
  }

  LeaveCriticalSection(*mutex);
  return 0;
}
