
================================================================================================#=
FILE:
    README.txt

LOCATION:
    <ProjRoot>/rabbitmq-c-lwip-freeertos/buildEmbedded/

DESCRIPTION:
    Directory for building a rabbitmq client library (static link)
    for a platform with LwIP and FreeRTOS.
================================================================================================#=

========================================================================#=
Instructions
========================================================================#=
cd buildEmbedded
rake       # default just shows list of available targets
rake -T    # task descriptions


rake librabbitmq > rake.log.txt 2>&1



========================================================================#=
Changes
========================================================================#=
----------------------------------------------------------------+-
These are the changes I had to make
 to get a clean build in quickstart.
----------------------------------------------------------------+-
(1) Custom build script using rake.
      Too much time and effort would be needed to understand cmake.
      Cheaper to just use make/rake based on the output of the cmake build.
      Changes to the include path to get the right platform stuff.

(2) Compilation cannot find include file: sys/uio.h
    The sys/uio.h header file contains definitions for vector I/O operations.
    RESOLUTION:
      Created rabbitmq-c-lwip-freertos/librabbitmq/sys/uio.h
      Copied from ???:
      License: Berkley/GNU
      Also available in the GNU C library.

(3) When compiling amqp_private.c
    Cannot find include file(s):
        arpa/inet.h
    RESOLUTION:
        Changed source file to include lwip/inet.h instead.

(4) When compiling amqp_socket.c
    Cannot find include file(s):
        netinet/in.h
        netinet/tcp.h
    RESOLUTION:
        Deleted these #includes - it appears they are not needed.

(5) When compiling amqp_socket.c
    Cannot find include file(s):
        sys/socket.h
    RESOLUTION:
        Changed source file to include lwip/sockets.h instead.

(6) When compiling amqp_socket.c
    Call to fcntl requires 3 arguments (LwIP); added 0 for third arg.

(7) When compiling amqp_socket.c
    Numerous errors due to the following structure members
    conflicting with macro names defined in lwip/sockets.h
          /** V-table for amqp_socket_t */
          struct amqp_socket_class_t {
            amqp_socket_writev_fn     writev;
            amqp_socket_send_fn       send;
            amqp_socket_recv_fn       recv;
            amqp_socket_open_fn       open;
            amqp_socket_close_fn      close;
            amqp_socket_get_sockfd_fn get_sockfd;
            amqp_socket_delete_fn     delete;
          };

    RESOLUTION:
        Changed member names by appending _sockfn to each member.

(8) When compiling amqp_socket.c
        Had to ensure LWIP_DNS is defined in lwipopts.h

(9) When compiling amqp_timer.c
        warning: implicit declaration of function 'clock_gettime' [-Wimplicit-function-declaration]
        error: 'CLOCK_MONOTONIC' undeclared
    RESOLUTION:
        Created rabbitmq-c-lwip-freertos/librabbitmq/posix/time.h
        with content (minimal) based on POSIX standard.
        Created rabbitmq-c-lwip-freertos/librabbitmq/posix/posix_time.c
        Added #include <posix/time.h>

(10) When linking...
    Missing symbols:
        amqp_tcp_socket.c:206: undefined reference to `recv'
        amqp_tcp_socket.c:132: undefined reference to `writev'
    RESOLUTION:
        Copied from https://github.com/macton/nativecolors/client/src/libraries/nativeblue/
        To:
            rabbitmq-c-lwip-freertos/librabbitmq/readv.c
            rabbitmq-c-lwip-freertos/librabbitmq/writev.c
        Added to rabbitmq makefile.

(11) When linking...
    Missing symbols:
        amqp_tcp_socket.c:206: undefined reference to `recv'
    RESOLUTION:
        Added #include lwip/sockets.h to amqp_tcp_socket.c



@@@ Also Note: some compilation warnings remain. @@@





