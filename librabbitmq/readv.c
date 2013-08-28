
/*
================================================================================================#=
FROM:
    https://github.com/macton/nativecolors/client/src/libraries/nativeblue/readv.c
================================================================================================#=
*/


#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

ssize_t readv(int d, const struct iovec *iov, int iovcnt)
{
  ssize_t total_read_len;
  int     i;
 
  total_read_len = 0;
  for (i=0;i<iovcnt;i++)
  {
    ssize_t read_len = read( d, iov[i].iov_base, iov[i].iov_len );
    if ( read_len != iov[i].iov_len )
    {
      // errno set by read
      return -1;
    }
    total_read_len += read_len;
  }

  return total_read_len;
}

