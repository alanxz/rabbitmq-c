#ifndef LIBRABBITMQ_STRDUP_H_
#define LIBRABBITMQ_STRDUP_H_
/* strdup is not in ISO C90!
 * we define it here for easy inclusion
 */
static inline char *strdup(const char *str)
{
  return strcpy(malloc(strlen(str) + 1),str);
}
#endif
