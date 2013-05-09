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

API Documentation (rather incomplete at this point) can be found:
- <http://alanxz.github.com/rabbitmq-c/docs/0.2/>

## Latest Stable Version

The latest stable version of rabbitmq-c is v0.3.0 and can be downloaded from:
https://github.com/alanxz/rabbitmq-c/archive/rabbitmq-c-v0.3.0.zip

## Building and installing using CMake

The rabbitmq-c library is built using CMake v2.6+ (http://www.cmake.org) on all
platforms.

The library itself requires no external dependancies.

*OPTIONALLY*: A set of command line tools used to interact with the broker are
included and require the Popt library (http://freecode.com/projects/popt).
A matching set of man pages have been written in DocBook format and need
the XmlTo (https://fedorahosted.org/xmlto/) utility to function correctly.

On most systems the commands to build rabbitmq-c are:

    mkdir build && cd build
    cmake ..
    cmake --build .

It is also possible to point the CMake GUI tool at the CMakeLists.txt in the root of
the source tree and generate build projects or IDE workspace

Installing the library and optionally specifying a prefix can be done with:

    cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
    cmake --build . --target install

More information on CMake can be found on its FAQ (http://www.cmake.org/Wiki/CMake_FAQ)

Other interesting flags that can be passed to CMake:
* `BUILD_EXAMPLES` - builds the example code
* `BUILD_TOOLS` builds some command line tools for interacting with the broker
* `BUILD_TOOLS_DOCS` is enabled and XmlTo is found
* `BUILD_SHARED_LIBS` - build rabbitmq-c as a shared library
* `BUILD_STATIC_LIBS` - build rabbitmq-c as a static library

## Building and installing using autotools

For legacy purposes, a GNU autotools based build system is also maintained. The required
utilities you need are autoconf v2.59+, automake v1.9+, libtool v2.2+, and pkg-config.

From a fresh tarball you will need to run reconf:

    autoreconf -i

Then the standard autotools build procedure will build rabbitmq-c:

    ./configure
    make
    make install

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
