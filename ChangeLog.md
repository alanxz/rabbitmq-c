# Change Log
## Changes since v0.4.1 (a.k.a., v0.5.0):
### Major changes:
- Add amqp_get_broker_properties() function 5c7c40adc1
- Remove distro-specific packaging a5749657ee
- Add -x flag to amqp-consume utilty 1d9c5291ff
- Add amqp_basic_nack() public API 9b168776fb
- Add delivery mode constants to amqp.h 5f291ea772
- Add support for connection.blocked/connection.unblocked methods ccbc24d270

### Bug fixes:
- `f8c6cee749` Examples: Destroy amqp_envelope_t in consumer example
- `ac88db56d3` CMake: fix generation of librabbitmq.pc
- `d5b35afa40` CMake: fix missing POPT_INCLUDE_DIRS variable in tools/
- `5ea6a0945a` build: provide independent locations for x64 libs
- `fac34656c0` Doc: documentation fixes
- `715901d675` Lib: Correct OpenSSL initialization under threaded conditions
- `ce64e57df8` Examples: Handle unexpected frames in amqp_consumer.c
- `bcda3e933d` CMake: Use GnuInstallDirs to generate install dirs
- `27245a4e15` Lib: correctly handle amqp_get_monotonic_timestamp on win32
- `693662ef5b` Tools: honor --persistent flag in publish utility
- `01d9c3ca60` Doc: improve documentation in amqp_ssl_socket functions
- `02d5c58ae4` autoconf: correct librabbitmq.pc generation
- `1f4e0cc48b` Doc: improve documentation in amqp_tcp_socket functions

## Changes since v0.4.0:
### Major changes:
- Removed distro-specific packaging d285d01

### Bug fixes:
- `a642602` FIX: destroy amqp_envelop_t object in consumer example
- `860dd71` FIX: correct generation of librabbitmq.pc under CMake
- `bdda7ab` FIX: amqp_socket_close() should not be exported from shlib
- `24f4131` FIX: Use correct buf/len vars when re-starting send()

## Changes since v0.3.0:
### New Features/Enhancements:
- `amqp_login_with_properties()` function to connect to a broker sending a
   properties table to the broker 21b124e #101
- SSL support (Mike Steinert) 473c865 #17
- `amqp_simple_wait_frame_noblock()` function variant to wait for a frame
   with a timeout f8cfc72 #119
- Allow memory to be released on a per-channel basis with
   `amqp_maybe_release_buffers_on_channel()` 4a2d899 #5
- Support for AMQP heartbeats while blocking in `amqp_simple_wait_frame*()`
   and `amqp_basic_publish()` daa0e66 aca5dc1
- `amqp_socket_open_noblock()` for a non-blocking socket connection
   (Bogdan Padalko) 6ad770d
- `amqp_table_clone()` to do a deep-copy of an amqp_table_t 08af83a
- Add option to listen to multiple keys in `amqp_consume` tool (Brian Hammond) e6c256d
- Add contributed OpenVMS build system 448ab68
- Higher level APIs for consuming messages 33ebeed #8
- Doxygen-based API documentation.
- Many improvements to error-handling and reporting

### Bug Fixes:
- `24ffaf8` FIX: autotools was broken when dependency-tracking was disabled
- `38e741b` FIX: CMake XmlTo not found warning
- `906f04f` FIX: htonll redeclared on Win32 v8
- `8e41603` FIX: SIGPIPE not disabled on OS X/BSD #102
- `872ea49` FIX: Header issues with amqp.h on Mingw on Win32 (yoniyoni)
- `0f1f75b` FIX: potential memory leak in amqp_new_connection
- `c9f6312` FIX: missing va_end in `amqp_login()`/`amqp_login_with_properties()`
- `7bb64e4` FIX: include amqp_tcp_socket.h in dpkg (Tim Stewart)
- `ba9d1f5` FIX: Report out of buffer space in `amqp_table_encode()`
- `9496e10` FIX: Remove `abort()` on invalid parameter in `amqp_send_frame()`
- `f209420` FIX: Remote `abort()` in `amqp_simple_wait_method()`
- `f027518` FIX: Return error on socket lib init error
- `0ae534a` FIX: Correctly handle 0 return val from `SSL_read()`/`SSL_write()`
- `22e41b8` FIX: Improve error handling in socket functions
- `33c2353` FIX: Set state->socket to NULL after `amqp_socket_close()`
- `c83e728` FIX: Incorrect error code returned
- `1a19879` FIX: redecl of int i in `amqp_tcp_socket_writev()`
- `7477449` FIX: incorrect bit-shift in `amqp_error_string2()`
- `2e37bb3` FIX: correctly handle `amqp_get_sockfd()` in `amqp_simple_wait_frame()`
- `52a459b` FIX: Don't delete state in `amqp_tune_connection()` on error
- `01e38dd` FIX: Correctly handle `mach_timebase_info()` failure
- `34bffb7` FIX: Correctly disable `SIGPIPE` on platforms with `SO_NOSIGPIPE`
- `3866433` FIX: Use correct number of bits in timer precision on MacOSX
- `b6a1dfe` FIX: Squash OpenSSL deprecated warnings on MacOSX (Bogdan Padalko)
- `7a217d5` FIX: Incorrect `assert()` in `wait_frame_inner()`
- `7942af3` FIX: Correctly handle 0-length table in `amqp_table_clone()`
- `157788e` FIX: Correctly handle 0-length strings in `amqp_basic_properties_clone()`
- `4eaf771` FIX: Correctly handle 0-length message body in `amqp_read_message()`
- `59f943b` FIX: Double-free SSL on connection failure
- `7a451a4` FIX: `amqp_open_socket()` not defined
