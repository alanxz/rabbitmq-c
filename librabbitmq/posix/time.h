

#include <sys/types.h>

#define CLOCK_MONOTONIC 42


int clock_gettime(clockid_t clk_id, struct timespec *tp);

