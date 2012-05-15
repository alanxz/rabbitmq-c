# RabbitMQ C AMQP client library

## Introduction

This is a C-language AMQP client library for use with AMQP servers
speaking protocol versions 0-9-1.

 - <http://www.rabbitmq.com/>
 - <http://www.amqp.org/>
 - <http://hg.rabbitmq.com/rabbitmq-c>

Announcements regarding the library are periodically made on the
RabbitMQ mailing list and on the RabbitMQ blog.

 - <http://lists.rabbitmq.com/cgi-bin/mailman/listinfo/rabbitmq-discuss>
 - <http://www.rabbitmq.com/blog/>

## Retrieving the code

In addition to the source code for this library, you will require a
copy of `rabbitmq-codegen`. Here is a short `sh` script for retrieving
the necessary pieces:

    git clone https://github.com/rabbitmq/rabbitmq-codegen/
    git clone https://github.com/alanxz/rabbitmq-c

You will also need a recent python with the simplejson module
installed, and the GNU autotools (autoconf, automake, libtool etc)
or as an alternative CMake

## Building the code

# Using autoconf

Once you have all the prerequisites, change to the `rabbitmq-c`
directory and run

    autoreconf -i

to run the GNU autotools and generate the configure script, followed
by

    ./configure
    make

to build the `librabbitmq` library and the example programs.

# Using cmake

You will need CMake: http://cmake.org/

You will need a working python install in your path.

If you would like the build system to fetch the rabbitmq-codegen
automatically you will need to have a working git install in your
path. Otherwise you will need to checkout the rabbitmq-codegen as
stated above.

Create a binary directory in a sibling directory from the directory
you cloned the rabbitmq-c repository

    mkdir bin-rabbitmq-c

Run CMake in the binary directory

    cmake /path/to/source/directory

Build it:

* On linux: `make`
* On win32: `nmake` or `msbuild`, or open it in visual studio and
  build from there

Things you can pass to cmake to change the build:

* `-DRABBITMQ_CODEGEN_DIR=/path/to/rabbitmq-codegen/checkout` - if you
   have your codegen directory in a different place [Default is
   sibiling directory to source]
* `-DFETCH_CODEGEN_FROM_GIT=ON` - if you want cmake to fetch the
   rabbitmq-codegen from https://github.com/rabbitmq/rabbitmq-codegen
   at build time. If this option is selected `-DRABBITMQ_CODEGEN_DIR`
   will be ignored [Default is off]
* `-DCODEGEN_GIT_TAG=rabbitmq_v2_5_1` - specifies the tag to check out
   if using the `-DFETCH_CODEGEN_FROM_GIT` option above. [Default is
   `rabbitmq_v2_5_1`]
* `-DBUILD_TOOLS=OFF` build the programs in the tools directory
    [Default is ON if the POPT library can be found]

Other interesting flags to pass to CMake (see cmake docs for more info)

* `-DCMAKE_BUILD_TYPE` - specify the type of build (Debug or Release)
* `-DCMAKE_INSTALL_PREFIX` - specify where the install target puts files

## Running the examples

Arrange for a RabbitMQ or other AMQP server to be running on
`localhost` at TCP port number 5672.

In one terminal, run

    ./examples/amqp_listen localhost 5672 amq.direct test

In another terminal,

    ./examples/amqp_sendstring localhost 5672 amq.direct test "hello world"

You should see output similar to the following in the listener's
terminal window:

    Result 1
    Frame type 1, channel 1
    Method AMQP_BASIC_DELIVER_METHOD
    Delivery 1, exchange amq.direct routingkey test
    Content-type: text/plain
    ----
    00000000: 68 65 6C 6C 6F 20 77 6F : 72 6C 64                 hello world
    0000000B:

## Writing applications using `librabbitmq`

Please see the `examples` directory for short examples of the use of
the `librabbitmq` library.

### Threading

You cannot share a socket, an `amqp_connection_state_t`, or a channel
between threads using `librabbitmq`. The `librabbitmq` library is
built with event-driven, single-threaded applications in mind, and
does not yet cater to any of the requirements of `pthread`ed
applications.

Your applications instead should open an AMQP connection (and an
associated socket, of course) per thread. If your program needs to
access an AMQP connection or any of its channels from more than one
thread, it is entirely responsible for designing and implementing an
appropriate locking scheme. It will generally be much simpler to have
a connection exclusive to each thread that needs AMQP service.
