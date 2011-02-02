#ifndef librabbitmq_strdup_h
#define librabbitmq_strdup_h
/* strdup is not in ISO C90!
 * we define it here for easy inclusion
 */
static inline char *strdup(const char *str)
{
  return strcpy(malloc(strlen(str) + 1),str);
}
#endif
