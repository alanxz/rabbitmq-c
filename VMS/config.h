/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 for a Win32 build. */
/* #undef AMQP_BUILD */

/* Define to 1 for a static Win32 build. */
/* #undef AMQP_STATIC */

/* Host operating system string */
#define AMQ_PLATFORM "openvms"

/* Define to 1 to enable thread safety */
#define ENABLE_THREAD_SAFETY 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if the function (or macro) htonll exists. */
/* #undef HAVE_HTONLL */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* "Have poll()" */
#define HAVE_POLL /**/

/* Define to 1 if Popt is available. */
/* #undef HAVE_POPT */

/* Define to 1 if you have the <popt.h> header file. */
/* #undef HAVE_POPT_H */

/* "Have select()" */
/* #undef HAVE_SELECT */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 for Win32. */
/* #undef OS_WIN32 */

/* Name of package */
#define PACKAGE "rabbitmq-c"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "https://github.com/alanxz/rabbitmq-c/issues"

/* Define to the full name of this package. */
#define PACKAGE_NAME "rabbitmq-c"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "rabbitmq-c 0.8.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "rabbitmq-c"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://www.rabbitmq.com/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.8.1"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 0
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif

/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif

/* Version number of package */
#define VERSION "0.8.1"

/* Define to 1 if SSL/TLS is enabled. */
#define WITH_SSL 1

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

