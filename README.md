# RabbitMQ C AMQP client library

[![Build Status](https://secure.travis-ci.org/alanxz/rabbitmq-c.png?branch=master)](http://travis-ci.org/alanxz/rabbitmq-c)

## Introduction

This is a C-language AMQP client library for use with AMQP servers
speaking protocol versions 0-9-1.

 - <http://www.rabbitmq.com/>
 - <http://www.amqp.org/>
 - <http://github.com/alanxz/rabbitmq-c>

Announcements regarding the library are periodically made on the
RabbitMQ mailing list and on the RabbitMQ blog.

 - <http://lists.rabbitmq.com/cgi-bin/mailman/listinfo/rabbitmq-discuss>
 - <http://www.rabbitmq.com/blog/>

## Retrieving the code

In addition to the source code for this library, you will require a
copy of `rabbitmq-codegen`, which resides in the `codegen` directory
as a git submodule. To update the submodule(s):

    git clone git://github.com/alanxz/rabbitmq-c.git
    cd rabbitmq-c
    git submodule init
    git submodule update

You will also need a recent python with the simplejson module
installed, and the GNU autotools (autoconf, automake, libtool etc.),
or as an alternative CMake.

## Building the code

### Using autoconf

Once you have all the prerequisites, change to the `rabbitmq-c`
directory and run

    autoreconf -i

to run the GNU autotools and generate the configure script, followed
by

    ./configure
    make

to build the `librabbitmq` library and the example programs.

### Using cmake

You will need CMake (v2.6 or better): http://cmake.org/

You will need a working python install (2.6+) with the json or simplejson
modules installed.

You will need to do the git submodule init/update as above.
Alternatively you can clone the rabbitmq-codegen repository and point
cmake to it using the RABBITMQ_CODEGEN_DIR cmake variable

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
