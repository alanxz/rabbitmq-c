
/*
================================================================================================#=
FILE:
  posix_time.c
================================================================================================#=
*/
#include "posix/time.h"


int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    // should also set errno
    return -1;
}
