# vim:set ts=2 sw=2 sts=2 et:
cmake_minimum_required(VERSION 2.6)
project(rabbitmq-c "C")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(RMQ_VERSION_MAJOR 0)
set(RMQ_VERSION_MINOR 3)
set(RMQ_VERSION_PATCH 0)

# Follow all steps below in order to calculate new ABI version when updating the library
# NOTE: THIS IS UNRELATED to the actual project version
#
# 1. If the library source code has changed at all since the last update, then increment revision
# 2. If any interfaces have been added, removed, or changed since the last update, increment current and set revision to 0.
# 3. If any interfaces have been added since the last public release, then increment age.
# 4. If any interfaces have been removed since the last public release, then set age to 0.

set(RMQ_SOVERSION_CURRENT   1)
set(RMQ_SOVERSION_REVISION  1)
set(RMQ_SOVERSION_AGE       0)

math(EXPR RMQ_SOVERSION_MAJOR "${RMQ_SOVERSION_CURRENT} - ${RMQ_SOVERSION_AGE}")
math(EXPR RMQ_SOVERSION_MINOR "${RMQ_SOVERSION_AGE}")
math(EXPR RMQ_SOVERSION_PATCH "${RMQ_SOVERSION_REVISION}")

set(RMQ_VERSION ${RMQ_SOVERSION_MAJOR}.${RMQ_SOVERSION_MINOR}.${RMQ_SOVERSION_PATCH})
set(RMQ_SOVERSION ${RMQ_SOVERSION_MAJOR})

set(VERSION ${RMQ_VERSION_MAJOR}.${RMQ_VERSION_MINOR}.${RMQ_VERSION_PATCH})

if (MSVC)
  set(CMAKE_C_FLAGS "/W4 /nologo ${CMAKE_C_FLAGS}")
elseif (CMAKE_COMPILER_IS_GNUCC)
  set(RMQ_C_FLAGS "-Wall -Wextra -pedantic -Wstrict-prototypes -Wcast-align -Wno-unused-function -fno-common")
  execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
  if (GCC_VERSION VERSION_GREATER 4.0 OR GCC_VERSION VERSION_EQUAL 4.0)
      set(RMQ_C_FLAGS "${RMQ_C_FLAGS} -fvisibility=hidden")
  endif()
  set(CMAKE_C_FLAGS "${RMQ_C_FLAGS} ${CMAKE_C_FLAGS}")
endif ()

if (CMAKE_GENERATOR MATCHES ".*(Make|Ninja).*"
    AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
  message(STATUS "CMAKE_BUILD_TYPE not specified. Creating ${CMAKE_BUILD_TYPE} build")
endif()

include(TestCInline)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(CMakePushCheckState)

# Detect if we need to link against a socket library:
cmake_push_check_state()
if (WIN32)
  # Always use WinSock2 on Windows
  set(SOCKET_LIBRARIES ws2_32)
else ()
  # Is it in the default link?
  CHECK_FUNCTION_EXISTS(getaddrinfo HAVE_GETADDRINFO)
  if (NOT (HAVE_GETADDRINFO EQUAL 1))
    CHECK_LIBRARY_EXISTS(socket getaddrinfo "" HAVE_GETADDRINFO2)
    if (HAVE_GETADDRINFO2 EQUAL 1)
      set(SOCKET_LIBRARIES socket)
    else ()
      CHECK_LIBRARY_EXISTS("socket;nsl" getaddrinfo "" HAVE_GETADDRINFO3)
      if (HAVE_GETADDRINFO3 EQUAL 1)
        set(SOCKET_LIBRARIES socket nsl)
      else ()
        message(FATAL_ERROR "Cannot find name resolution library (containing symbol getaddrinfo)")
      endif ()
    endif ()
  endif ()

  set(CMAKE_REQUIRED_LIBRARIES ${SOCKET_LIBRARIES})
  CHECK_FUNCTION_EXISTS(socket HAVE_SOCKET)
  if (NOT HAVE_SOCKET EQUAL 1)
    CHECK_LIBRARY_EXISTS(socket socket "" HAVE_SOCKET2)
    if (NOT HAVE_SOCKET2 EQUAL 1)
      set(SOCKET_LIBRARIES socket ${SOCKET_LIBRARIES})
    else ()
      CHECK_LIBRARY_EXISTS("socket;nsl" socket "" HAVE_SOCKET3)
      if (HAVE_SOCKET3 EQUAL 1)
        set(SOCKET_LIBRARIES socket nsl ${SOCKET_LIBRARIES})
      else ()
        message(FATAL_ERROR "Cannot find socket library (containing symbol socket)")
      endif ()
    endif ()
  endif ()
endif ()
cmake_pop_check_state()

cmake_push_check_state()
set(CMAKE_REQUIRED_LIBRARIES ${SOCKET_LIBRARIES})
if (WIN32)
  check_symbol_exists(htonll Winsock2.h HAVE_HTONLL)
else (WIN32)
  check_function_exists(htonll HAVE_HTONLL)
endif (WIN32)
cmake_pop_check_state()

check_library_exists(rt clock_gettime "time.h" CLOCK_GETTIME_NEEDS_LIBRT)
if (CLOCK_GETTIME_NEEDS_LIBRT)
  set(LIBRT rt)
endif()

option(REGENERATE_AMQP_FRAMING "Regenerate amqp_framing.h/amqp_framing.c sources (for developer use)" OFF)
mark_as_advanced(REGENERATE_AMQP_FRAMING)

if (REGENERATE_AMQP_FRAMING)
  find_package(PythonInterp)
  if (NOT PYTHONINTERP_FOUND)
    message(FATAL_ERROR "REGENERATE_AMQP_FRAMING requires Python to be available")
  endif ()

  #Determine Python Version:
  if(PYTHON_EXECUTABLE)
    execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c
      "import sys; sys.stdout.write(';'.join([str(x) for x in sys.version_info[:3]]))"
      OUTPUT_VARIABLE _VERSION
      RESULT_VARIABLE _PYTHON_VERSION_RESULT
      ERROR_QUIET)
    if(NOT _PYTHON_VERSION_RESULT)
      string(REPLACE ";" "." PYTHON_VERSION_STRING "${_VERSION}")
      list(GET _VERSION 0 PYTHON_VERSION_MAJOR)
      list(GET _VERSION 1 PYTHON_VERSION_MINOR)
      list(GET _VERSION 2 PYTHON_VERSION_PATCH)
      if(PYTHON_VERSION_PATCH EQUAL 0)
        # it's called "Python 2.7", not "2.7.0"
        string(REGEX REPLACE "\\.0$" "" PYTHON_VERSION_STRING "${PYTHON_VERSION_STRING}")
      endif()
    else()
      # sys.version predates sys.version_info, so use that
      execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c "import sys; sys.stdout.write(sys.version)"
        OUTPUT_VARIABLE _VERSION
        RESULT_VARIABLE _PYTHON_VERSION_RESULT
        ERROR_QUIET)
      if(NOT _PYTHON_VERSION_RESULT)
        string(REGEX REPLACE " .*" "" PYTHON_VERSION_STRING "${_VERSION}")
        string(REGEX REPLACE "^([0-9]+)\\.[0-9]+.*" "\\1" PYTHON_VERSION_MAJOR "${PYTHON_VERSION_STRING}")
        string(REGEX REPLACE "^[0-9]+\\.([0-9])+.*" "\\1" PYTHON_VERSION_MINOR "${PYTHON_VERSION_STRING}")
        if(PYTHON_VERSION_STRING MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+.*")
          string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" PYTHON_VERSION_PATCH "${PYTHON_VERSION_STRING}")
        else()
          set(PYTHON_VERSION_PATCH "0")
        endif()
      else()
        # sys.version was first documented for Python 1.5, so assume
        # this is older.
        set(PYTHON_VERSION_STRING "1.4")
        set(PYTHON_VERSION_MAJOR "1")
        set(PYTHON_VERSION_MAJOR "4")
        set(PYTHON_VERSION_MAJOR "0")
      endif()
    endif()
    unset(_PYTHON_VERSION_RESULT)
    unset(_VERSION)
  endif(PYTHON_EXECUTABLE)

  # If we're running v3.x look for a 2to3 utility
  if (PYTHON_VERSION_MAJOR GREATER 2)
    get_filename_component(PYTHON_EXE_DIR ${PYTHON_EXECUTABLE} PATH)
    find_program(PYTHON_2TO3_EXECUTABLE
      NAMES 2to3
      HINTS ${PYTHON_EXE_DIR}
      )

    if ("PYTHON_2TO3_EXECUTABLE-NOTFOUND" STREQUAL PYTHON_2TO3_EXECUTABLE)
      message(FATAL_ERROR "Unable to find 2to3 python utility, specify python 2.7 or specify 2to3 utility")
    endif ()
  endif (PYTHON_VERSION_MAJOR GREATER 2)


  #check for json or simplejson
  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import json"
    RESULT_VARIABLE CHECK_PYTHON_JSON_FAILED
  )

  if (CHECK_PYTHON_JSON_FAILED)
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import simplejson"
      RESULT_VARIABLE CHECK_PYTHON_SIMPLEJSON_FAILED
    )

    if (CHECK_PYTHON_SIMPLEJSON_FAILED)
      message(FATAL_ERROR "REGENERATE_AMQP_FRAMING requires a python with json or simplejson modules")
    endif (CHECK_PYTHON_SIMPLEJSON_FAILED)
  endif (CHECK_PYTHON_JSON_FAILED)


  find_path(AMQP_CODEGEN_DIR
    amqp_codegen.py
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/codegen
          ${CMAKE_CURRENT_SOURCE_DIR}/rabbitmq-codegen 
          ${CMAKE_CURRENT_SOURCE_DIR}/../rabbitmq-codegen
          DOC "Path to directory containing amqp_codegen.py (rabbitmq-codegen)"
          NO_DEFAULT_PATH
          )

  if (AMQP_CODEGEN_DIR STREQUAL "AMQP_CODEGEN_DIR-NOTFOUND")
    message(SEND_ERROR "REGENERATE_AMQP_FRAMING requires the amqp_codegen.py script. If this is a git clone you can:\n\ngit submodule init\ngit submodule update\n\n Or set AMQP_CODEGEN_DIR to directory containing amqp_codegen.py")
  else ()
    message(STATUS "Found amqp_codegen.py in ${AMQP_CODEGEN_DIR}")
  endif()
endif (REGENERATE_AMQP_FRAMING)

find_package(POPT)
find_package(XmlTo)

if (POPT_FOUND AND XmlTo_FOUND)
  set(DO_DOCS ON)
endif()

find_package(Threads)

option(BUILD_SHARED_LIBS "Build rabbitmq-c as a shared library" ON)
option(BUILD_STATIC_LIBS "Build rabbitmq-c as a static library" OFF)

option(BUILD_EXAMPLES "Build Examples" ON)
option(BUILD_TOOLS "Build Tools (requires POPT Library)" ${POPT_FOUND})
option(BUILD_TOOLS_DOCS "Build man pages for Tools (requires xmlto)" ${DO_DOCS})
option(BUILD_TESTS "Build tests (run tests with make test)" ON)
option(ENABLE_SSL_SUPPORT "Enable SSL support" ON)
option(ENABLE_THREAD_SAFETY "Enable thread safety when using OpenSSL" ${Threads_FOUND})

set(SSL_ENGINE "OpenSSL" CACHE STRING "SSL Backend to use, valid options: OpenSSL, cyaSSL, GnuTLS, PolarSSL")
mark_as_advanced(SSL_ENGINE)

if (ENABLE_SSL_SUPPORT)
  if (SSL_ENGINE STREQUAL "OpenSSL")
    find_package(OpenSSL 0.9.8 REQUIRED)

  elseif (SSL_ENGINE STREQUAL "cyaSSL")
    find_package(cyaSSL REQUIRED)

  elseif (SSL_ENGINE STREQUAL "GnuTLS")
    find_package(GnuTLS REQUIRED)

  elseif (SSL_ENGINE STREQUAL "PolarSSL")
    find_package(PolarSSL REQUIRED)

  else()
    message(FATAL_ERROR "Unsupported SSL_ENGINE ${SSL_ENGINE}, valid engines: OpenSSL, cyaSSL, GnuTLS, or PolarSSL")
  endif()
endif()

if (NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)
    message(FATAL_ERROR "One or both of BUILD_SHARED_LIBS or BUILD_STATIC_LIBS must be set to ON to build")
endif()

if (WIN32 AND BUILD_STATIC_LIBS)
  message(FATAL_ERROR "The rabbitmq-c library cannot be built as a static library on Win32. Set BUILD_STATIC_LIBS=OFF to get around this.")
endif()

add_subdirectory(librabbitmq)

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif ()

if (BUILD_TOOLS)
  if (POPT_FOUND)
    add_subdirectory(tools)
  else ()
    message(WARNING "POpt library was not found. Tools will not be built")
  endif ()
endif ()

if (BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif (BUILD_TESTS)

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX}/bin)
set(libdir ${CMAKE_INSTALL_PREFIX}/lib)
set(includedir ${CMAKE_INSTALL_PREFIX}/include)

configure_file(cmake/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/librabbitmq/config.h)
configure_file(librabbitmq.pc.in ${CMAKE_CURRENT_BINARY_DIR}/librabbitmq.pc @ONLY)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/librabbitmq.pc
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig
  )

if (BUILD_SHARED_LIBS)
  message(STATUS "Building rabbitmq as a shared library - yes")
else ()
  message(STATUS "Building rabbitmq as a shared library - no")
endif ()

if (BUILD_STATIC_LIBS)
  message(STATUS "Building rabbitmq as a static library - yes")
else ()
  message(STATUS "Building rabbitmq as a static library - no")
endif ()
