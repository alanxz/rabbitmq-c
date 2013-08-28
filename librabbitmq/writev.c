
/*
================================================================================================#=
FROM:
    https://github.com/macton/nativecolors/client/src/libraries/nativeblue/writev.c
================================================================================================#=
*/



#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

ssize_t writev(int d, const struct iovec *iov, int iovcnt)
{
  ssize_t total_write_len;
  int     i;
 
  total_write_len = 0;
  for (i=0;i<iovcnt;i++)
  {
    ssize_t write_len = write( d, iov[i].iov_base, iov[i].iov_len );
    if ( write_len != iov[i].iov_len )
    {
      // errno set by write
      return -1;
    }
    total_write_len += write_len;
  }

  return total_write_len;
}


